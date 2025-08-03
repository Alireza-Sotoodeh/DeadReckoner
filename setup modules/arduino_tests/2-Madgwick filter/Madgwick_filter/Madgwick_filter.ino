#include <Wire.h>
#include <math.h>

#define PI 3.14159265358979323846f
#define DEG_TO_RAD (PI / 180.0f)

// MPU9250 registers
#define MPU9250_ADDRESS 0x68
#define AK8963_ADDRESS 0x0C
#define WHO_AM_I_MPU9250 0x75
#define PWR_MGMT_1 0x6B
#define SMPLRT_DIV 0x19
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define INT_PIN_CFG 0x37
#define INT_ENABLE 0x38
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43
#define USER_CTRL 0x6A
#define FIFO_EN 0x23

// AK8963 registers
#define AK8963_WHO_AM_I 0x00
#define AK8963_CNTL 0x0A
#define AK8963_ASAX 0x10
#define MAG_XOUT_L 0x03

// Quaternion variables for Madgwick filter
float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};  // [w, x, y, z]

// Filter parameters (tuned for 10 Hz update rate)
const float beta = 0.1f;  // Madgwick gain
const float zeta = 0.0f;  // Not using gyro bias correction here
const float deltat = 0.1f;  // 1 / update rate (10 Hz)

// Calibration offsets (example values; calibrate your sensor)
float accelBias[3] = {0.0f, 0.0f, 0.0f};
float gyroBias[3] = {0.0f, 0.0f, 0.0f};
float magBias[3] = {0.0f, 0.0f, 0.0f};
float magScale[3] = {1.0f, 1.0f, 1.0f};

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();

  // Wake up MPU9250
  writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x00);
  delay(100);

  // Verify MPU9250
  byte whoami = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  if (whoami != 0x71 && whoami != 0x73) {
    Serial.println("MPU9250 not detected!");
    while (1);
  }

  // Set sample rate to 10 Hz (1000Hz / (1 + 99) = 10Hz)
  writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 99);

  // Set gyro range to +/- 250 dps
  writeByte(MPU9250_ADDRESS, GYRO_CONFIG, 0x00);

  // Set accel range to +/- 2g
  writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, 0x00);

  // Enable I2C bypass to access AK8963 directly
  writeByte(MPU9250_ADDRESS, INT_PIN_CFG, 0x02);
  delay(100);

  // Verify AK8963
  whoami = readByte(AK8963_ADDRESS, AK8963_WHO_AM_I);
  if (whoami != 0x48) {
    Serial.println("AK8963 not detected!");
    while (1);
  }

  // Set magnetometer to continuous mode, 16-bit, 100Hz (but we'll read at 10Hz)
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x16);

  // Read factory mag sensitivity
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x0F);  // Fuse ROM access mode
  delay(10);
  byte rawData[3];
  readBytes(AK8963_ADDRESS, AK8963_ASAX, 3, rawData);
  magScale[0] = (float)(rawData[0] - 128) / 256.0f + 1.0f;
  magScale[1] = (float)(rawData[1] - 128) / 256.0f + 1.0f;
  magScale[2] = (float)(rawData[2] - 128) / 256.0f + 1.0f;
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00);  // Power down
  delay(10);
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x16);  // Back to continuous

  Serial.println("MPU9250 ready. Outputting quaternions (w,x,y,z)...");
}

void loop() {
  // Read accelerometer (resolution 2g / 32768)
  int16_t rawAccel[3];
  readAccelData(rawAccel);
  float ax = (float)rawAccel[0] / 16384.0f - accelBias[0];
  float ay = (float)rawAccel[1] / 16384.0f - accelBias[1];
  float az = (float)rawAccel[2] / 16384.0f - accelBias[2];

  // Read gyroscope (resolution 250 dps / 32768)
  int16_t rawGyro[3];
  readGyroData(rawGyro);
  float gx = (float)rawGyro[0] / 131.0f - gyroBias[0];
  float gy = (float)rawGyro[1] / 131.0f - gyroBias[1];
  float gz = (float)rawGyro[2] / 131.0f - gyroBias[2];

  // Read magnetometer (resolution 4912 uT / 32760 for 16-bit)
  int16_t rawMag[3];
  readMagData(rawMag);
  float mx = (float)rawMag[0] * magScale[0] - magBias[0];
  float my = (float)rawMag[1] * magScale[1] - magBias[1];
  float mz = (float)rawMag[2] * magScale[2] - magBias[2];

  // Normalize accel and mag
  float norm = sqrt(ax*ax + ay*ay + az*az);
  if (norm > 0) { ax /= norm; ay /= norm; az /= norm; }
  norm = sqrt(mx*mx + my*my + mz*mz);
  if (norm > 0) { mx /= norm; my /= norm; mz /= norm; }

  // Convert gyro to radians/sec
  gx *= DEG_TO_RAD;
  gy *= DEG_TO_RAD;
  gz *= DEG_TO_RAD;

  // Update quaternion with Madgwick filter
  MadgwickQuaternionUpdate(ax, ay, az, gx, gy, gz, my, mx, -mz);  // Note: mag axes may need swapping based on orientation

  // Output quaternion as CSV
  Serial.print(q[0], 6);
  Serial.print(",");
  Serial.print(q[1], 6);
  Serial.print(",");
  Serial.print(q[2], 6);
  Serial.print(",");
  Serial.println(q[3], 6);

  delay(100);  // 10 Hz update
}

// Madgwick filter implementation (9DOF)
void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz) {
  float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];
  float norm;
  float hx, hy, bx, bz;
  float s1, s2, s3, s4;
  float qDot1, qDot2, qDot3, qDot4;

  // Auxiliary variables to avoid repeated arithmetic
  float _2q1mx, _2q1my, _2q1mz, _2q2mx;
  float _2bx, _2bz, _4bx, _4bz;
  float _2q1 = 2.0f * q1;
  float _2q2 = 2.0f * q2;
  float _2q3 = 2.0f * q3;
  float _2q4 = 2.0f * q4;
  float _2q1q3 = 2.0f * q1 * q3;
  float _2q3q4 = 2.0f * q3 * q4;
  float q1q1 = q1 * q1;
  float q1q2 = q1 * q2;
  float q1q3 = q1 * q3;
  float q1q4 = q1 * q4;
  float q2q2 = q2 * q2;
  float q2q3 = q2 * q3;
  float q2q4 = q2 * q4;
  float q3q3 = q3 * q3;
  float q3q4 = q3 * q4;
  float q4q4 = q4 * q4;

  // Normalize accelerometer
  norm = sqrt(ax * ax + ay * ay + az * az);
  if (norm == 0.0f) return;
  norm = 1.0f / norm;
  ax *= norm;
  ay *= norm;
  az *= norm;

  // Normalize magnetometer
  norm = sqrt(mx * mx + my * my + mz * mz);
  if (norm == 0.0f) return;
  norm = 1.0f / norm;
  mx *= norm;
  my *= norm;
  mz *= norm;

  // Reference direction of Earth's magnetic field
  _2q1mx = 2.0f * q1 * mx;
  _2q1my = 2.0f * q1 * my;
  _2q1mz = 2.0f * q1 * mz;
  _2q2mx = 2.0f * q2 * mx;
  hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
  hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + _2q3 * my * q4 - _2q4 * mz * q2 - my * q3q3 + my * q4q4;
  _2bx = sqrt(hx * hx + hy * hy);
  _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - _2q3 * my * q2 + _2q4 * mx * q3 - mz * q2q2 - mz * q3q3 + _2q4 * my * q3 + mz * q4q4;
  _4bx = 2.0f * _2bx;
  _4bz = 2.0f * _2bz;

  // Gradient descent algorithm corrective step
  s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  norm = sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);
  norm = 1.0f / norm;
  s1 *= norm;
  s2 *= norm;
  s3 *= norm;
  s4 *= norm;

  // Compute rate of change of the orientation quaternion
  qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
  qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
  qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
  qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

  // Integrate to yield quaternion
  q[0] += qDot1 * deltat;
  q[1] += qDot2 * deltat;
  q[2] += qDot3 * deltat;
  q[3] += qDot4 * deltat;
  norm = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  norm = 1.0f / norm;
  q[0] *= norm;
  q[1] *= norm;
  q[2] *= norm;
  q[3] *= norm;
}

// Helper functions
void writeByte(byte address, byte subAddress, byte data) {
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.write(data);
  Wire.endTransmission();
}

byte readByte(byte address, byte subAddress) {
  byte data;
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.endTransmission();
  Wire.requestFrom(address, (byte)1);
  data = Wire.read();
  return data;
}

void readBytes(byte address, byte subAddress, byte count, byte * dest) {
  Wire.beginTransmission(address);
  Wire.write(subAddress);
  Wire.endTransmission();
  Wire.requestFrom(address, count);
  for (byte i = 0; i < count; i++) {
    dest[i] = Wire.read();
  }
}

void readAccelData(int16_t * dest) {
  byte rawData[6];
  readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, rawData);
  dest[0] = ((int16_t)rawData[0] << 8) | rawData[1];
  dest[1] = ((int16_t)rawData[2] << 8) | rawData[3];
  dest[2] = ((int16_t)rawData[4] << 8) | rawData[5];
}

void readGyroData(int16_t * dest) {
  byte rawData[6];
  readBytes(MPU9250_ADDRESS, GYRO_XOUT_H, 6, rawData);
  dest[0] = ((int16_t)rawData[0] << 8) | rawData[1];
  dest[1] = ((int16_t)rawData[2] << 8) | rawData[3];
  dest[2] = ((int16_t)rawData[4] << 8) | rawData[5];
}

void readMagData(int16_t * dest) {
  byte rawData[7];  // x/y/z + status
  readBytes(AK8963_ADDRESS, MAG_XOUT_L, 7, rawData);
  if (!(rawData[6] & 0x08)) {  // Check if data is not overflowed
    dest[0] = ((int16_t)rawData[1] << 8) | rawData[0];
    dest[1] = ((int16_t)rawData[3] << 8) | rawData[2];
    dest[2] = ((int16_t)rawData[5] << 8) | rawData[4];
  }
}