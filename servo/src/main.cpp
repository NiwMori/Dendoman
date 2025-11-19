#include <PS4Controller.h>
#include <ESP32Servo.h>

// サーボピン
const int servoLeftPin = 13;
const int servoRightPin = 14;

// 非常停止ボタンピン（任意で変更OK）
const int estopPin = 27;   // 例：GPIO27を使用

// サーボオブジェクト
Servo servoLeft;
Servo servoRight;

unsigned long lastTimeStamp = 0;

// 左サーボ設定
const int leftCenter = 80;
const int leftUpRange = 70;
const int leftDownRange = 70;

// 右サーボ設定
const int rightCenter = 115;
const int rightUpRange = 70;
const int rightDownRange = 95;

// デッドゾーン付きマッピング（上下別範囲）
int stickToAngle(int value, int center, int upRange, int downRange) {
  const int deadzone = 10;
  if (value > -deadzone && value < deadzone) return center; // 中立保持

  if (value > 0) {
    // スティック下方向（＋側）
    return map(value, 0, 127, center, center + downRange);
  } else {
    // スティック上方向（−側）
    return map(value, 0, -128, center, center - upRange);
  }
}

void onConnect() { Serial.println("Connected!"); }
void onDisConnect() { Serial.println("Disconnected!"); }

void setup() {
  Serial.begin(115200);

  // サーボ初期化
  servoLeft.attach(servoLeftPin);
  servoRight.attach(servoRightPin);

  // 非常停止ボタン入力設定（内部プルアップ）
  pinMode(estopPin, INPUT_PULLUP);
  // → ボタンが押されると LOW になる回路にする（安全かつノイズ耐性あり）

  // PS4初期化
  PS4.attachOnConnect(onConnect);
  PS4.attachOnDisconnect(onDisConnect);
  PS4.begin();

  Serial.println("Ready.");
}

void loop() {
  // 非常停止チェック（押されたら強制停止）
  bool estopPressed = (digitalRead(estopPin) == HIGH);

  if (estopPressed) {
    // 非常停止時は即中立角に戻す
    servoLeft.write(leftCenter);
    servoRight.write(rightCenter);

    if (millis() - lastTimeStamp > 500) {
      Serial.println("⚠️ EMERGENCY STOP PRESSED! Servos stopped.");
      lastTimeStamp = millis();
    }
    return; // ここで処理終了（PS4入力を無視）
  }

  // ===== 通常動作 =====
  if (PS4.isConnected()) {

    // 🔒 R1が押されているときだけ操作を有効化
    if (PS4.R1()) {
      // int angleLeft = stickToAngle(PS4.LStickY(), leftCenter, leftUpRange, leftDownRange);
      // int angleRight = stickToAngle(-PS4.RStickY(), rightCenter, rightUpRange, rightDownRange);
      int angleLeft = stickToAngle(-PS4.RStickY(), leftCenter, leftUpRange, leftDownRange);
      int angleRight = stickToAngle(PS4.LStickY(), rightCenter, rightUpRange, rightDownRange);


      servoLeft.write(angleLeft);
      servoRight.write(angleRight);

      if (millis() - lastTimeStamp > 200) {
        Serial.print("[R1 HELD] Left: "); Serial.print(angleLeft);
        Serial.print(" | Right: "); Serial.println(angleRight);
        lastTimeStamp = millis();
      }
    } 
    else {
      // R1を離している間は中立に戻す
      servoLeft.write(leftCenter);
      servoRight.write(rightCenter);

      if (millis() - lastTimeStamp > 500) {
        Serial.println("[R1 RELEASED] Servos in neutral.");
        lastTimeStamp = millis();
      }
    }
  }
}
