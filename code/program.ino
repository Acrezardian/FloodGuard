#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <time.h>

// Mendefinisikan pin sensor ultrasonik
#define TRIG_PIN D5
#define ECHO_PIN D6

// Mendefinisikan pin untuk sensor hujan
#define POWER_PIN D7  // Pin ESP8266 yang menyediakan daya ke sensor hujan
#define AO_PIN    A0  // Pin ESP8266 yang terhubung ke pin AO sensor hujan

// Mendefinisikan pin untuk LED dan Buzzer
#define GREEN_LED_PIN D1
#define YELLOW_LED_PIN D2
#define RED_LED_PIN D3
#define BUZZER_PIN D4

// Isikan sesuai pada Firebase
#define FIREBASE_HOST "" // MASUKKAN URL HOST DISINI tanpa https
#define FIREBASE_AUTH "" // MASUKKAN KODE SECRET DISINI

// Nama Wifi
#define WIFI_SSID "E5573-MIFI-236188"
#define WIFI_PASSWORD "12345678"

// mendeklarasikan objek data dari FirebaseESP8266
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Function to configure NTP time
void configTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); 
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}
void setup() {
  Serial.begin(115200);

  // Inisialisasi pin sensor ultrasonik
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Inisialisasi pin sensor hujan
  pinMode(POWER_PIN, OUTPUT);

  // Inisialisasi pin LED dan Buzzer
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

 digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);


  // Koneksi ke WiFi
  WifiConnect();
  // Configure NTP time
  configTime();

  // Konfigurasi Firebase
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  config.host = FIREBASE_HOST;
  
  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
}

void WifiConnect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Memulai menghubungkan ke wifi router
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Koneksi WiFi berhasil!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

float readUltrasonicDistance() {
  // Kirim pulse ke TRIG_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Hitung durasi pulse di ECHO_PIN
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Konversi durasi ke jarak dalam cm
  float distance = duration * 0.034 / 2;
  return distance;
}

String readRainSensor() {
  digitalWrite(POWER_PIN, HIGH);  // Nyalakan daya sensor hujan
  delay(10);                      // Tunggu 10 milidetik untuk menstabilkan sensor

  int analogValue = analogRead(AO_PIN);  // Baca nilai analog dari sensor hujan

  digitalWrite(POWER_PIN, LOW);   // Matikan daya sensor hujan
  delay(10);                      // Tunggu sejenak setelah mematikan daya

  // Tentukan kategori hujan berdasarkan nilai analog yang dibaca
  String rainStatus;
  if (analogValue > 700) {
    rainStatus = "Badai";
  } else if (analogValue > 350) {
    rainStatus = "Hujan";
  } else {
    rainStatus = "Cerah";
  }
  return rainStatus;
}

void controlLEDandBuzzer(float distance) {
  if (distance >= 15) {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Firebase.setString(firebaseData, "/Data/led", "Aman");
  } else if (distance >= 5 && distance < 15) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Firebase.setString(firebaseData, "/Data/led", "Siaga");
  } else if  (distance < 5 ) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Firebase.setString(firebaseData, "/Data/led", "Evakuasi");}
}

void HistoryLEDandBuzzer(float distance) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  // Create date and time strings
  char dateBuffer[11]; // Buffer for date in "YYYY-MM-DD" format
  strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
  String currentDate = String(dateBuffer);

  char timeBuffer[9]; // Buffer for time in "HH:MM:SS" format
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
  String currentTime = String(timeBuffer);

  // Construct the path for Firebase history
  String tanggal = "/History/" + currentDate + "/" + currentTime;

  if (distance >= 15) {
    Firebase.setString(firebaseData, tanggal + "/led", "Aman");
  } else if (distance >= 5 && distance < 15) {
    Firebase.setString(firebaseData, tanggal + "/led", "Siaga");
  } else if  (distance < 5 ) {
    Firebase.setString(firebaseData, tanggal + "/led", "Evakuasi");}
}


void loop() {

  // Get current time
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  
  char dateBuffer[11]; // Buffer for date in "YYYY-MM-DD" format
  strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
  String currentDate = String(dateBuffer);

  char timeBuffer[6]; // Buffer for time in "HH:MM" format
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
  String currentTime = String(timeBuffer);

  // Baca jarak dari sensor ultrasonik
  float distance = readUltrasonicDistance();

  // Baca status hujan dari sensor hujan
  String rainStatus = readRainSensor();

  // Tampilkan jarak dan status hujan di serial monitor
  Serial.print("Jarak: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("Status Hujan: ");
  Serial.println(rainStatus);
  
  Serial.print("Time: ");
  Serial.println(currentTime);
  Serial.println();

  // Kirim data jarak ke Firebase
  if (Firebase.setFloat(firebaseData, "/Data/jarak", distance)) {
    Serial.println("Jarak terkirim");
  } else {
    Serial.println("Jarak gagal terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }

  // Kirim data status hujan ke Firebase
  if (Firebase.setString(firebaseData, "/Data/cuaca", rainStatus)) {
    Serial.println("Status hujan terkirim");
  } else {
    Serial.println("Status hujan gagal terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }

 // Create Firebase path based on date and time
  String tanggal = "/History/" + currentDate + "/" + currentTime;
 // Kirim data jarak ke Firebase
  if (Firebase.setFloat(firebaseData,  tanggal +  "/jarak", distance)) {
    Serial.println("Jarak terkirim");
  } else {
    Serial.println("Jarak gagal terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }

  // Kirim data status hujan ke Firebase
  if (Firebase.setString(firebaseData,  tanggal +  "/cuaca", rainStatus)) {
    Serial.println("Status hujan terkirim");
  } else {
    Serial.println("Status hujan gagal terkirim");
    Serial.println("Karena: " + firebaseData.errorReason());
  }
     // Kontrol LED dan Buzzer berdasarkan jarak
     controlLEDandBuzzer(distance);
     HistoryLEDandBuzzer(distance);

  delay(1000); // Tunggu selama 1 detik
}
