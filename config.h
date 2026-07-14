/*
 * AutoCamp Configuration Header
 * Centralized constants and macros
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============ SYSTEM ============
#define SYSTEM_NAME "AutoCamp v0.2"
#define BOOT_MODEM 30
#define ENABLE_AUDIO  // Comment to disable MP3

// ============ HARDWARE PINS ============
#if defined(__AVR_ATmega2560__)
  #define CE_PIN  42
  #define CSN_PIN 43
  #define PCB_TYPE "MEGA"
#else
  #define CE_PIN  9
  #define CSN_PIN 10
  #define PCB_TYPE "UNO"
#endif

// RF24 Configuration
#define CHANNEL 118
#define RF24_TIMEOUT_MS 50

// ============ DISPLAY ============
#define LCD_W  480
#define LCD_H  320

// Display control pins
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

// Touch screen pins
#define YP A7
#define XM A6
#define YM 48
#define XP 49
#define TS_MINX 240
#define TS_MAXX 945
#define TS_MINY 205
#define TS_MAXY 780
#define MINPRESSURE 5

// ============ LAYOUT ============
#define BUTTON_C BLUE
#define BUTTON_W 100
#define BUTTON_H 80
#define BUTTON_ROUND 8

#define BUTTON_TW 100
#define BUTTON_TH 40

#define CHK_W 35
#define CHK_H 35

#define GRAPH_X 40
#define GRAPH_Y 300
#define GRAPH_W 440
#define GRAPH_H 200

#define TOUCH_MAX 40
#define BUFFPIXEL 20

// ============ SENSORS ============
#define MaxSensors 16
#define MaxSensorsData24H 24
#define MaxSensorsData1H 12
#define SENSORS_KEY 123

// Sensor types
#define T_VOLT 0
#define T_TEMP 1
#define T_PROC 2
#define T_ANALOG 3
#define T_OUTS 4
#define T_PWM 5
#define T_PAGE 10
#define T_SETUP 11
#define T_ALARM 12

// Sensor options
#define O_ACTIVE 1
#define O_ALARM 2
#define O_RAPORT 3

// ============ TIMING ============
#define REFRESH_SENSORS 5000
#define UNIX_DAY 86400L

// ============ AUDIO ============
#define P_WELCOME 0x0101
#define P_CLICK 0x010A

#define CMD_NEXT_SONG     0X01
#define CMD_PREV_SONG     0X02
#define CMD_PLAY_W_INDEX  0X03
#define CMD_VOLUME_UP     0X04
#define CMD_VOLUME_DOWN   0X05
#define CMD_SET_VOLUME    0X06
#define CMD_SNG_CYCL_PLAY 0X08
#define CMD_SEL_DEV       0X09
#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
#define CMD_PLAY          0X0D
#define CMD_PAUSE         0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F
#define CMD_STOP_PLAY     0X16
#define CMD_FOLDER_CYCLE  0X17
#define CMD_SHUFFLE_PLAY  0x18
#define CMD_SET_SNGL_CYCL 0X19
#define CMD_SET_DAC 0X1A
#define DAC_ON  0X00
#define DAC_OFF 0X01
#define CMD_PLAY_W_VOL    0X22
#define CMD_PLAYING_N     0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f
#define DEV_TF            0X02

// ============ REFRESH FLAGS ============
#define R_NONE 0
#define R_FULL 1
#define R_UPDATE 2
#define R_DRAW 3

// ============ SCREEN IDS ============
#define E_MAIN 1
#define E_DEVICE 2
#define E_SETUP 3
#define E_GRAPH 4
#define E_SOUND 5
#define E_SENSORS 6
#define E_SENSOR 7
#define E_ICONS 8
#define E_MODEM 9
#define E_DATETIME 10

// ============ BUTTON IDs (NEGATIVE) ============
#define K_ENTER -1
#define K_CANCEL -2
#define K_DEL -3
#define K_TAK -4
#define K_NIE -5
#define K_MENU -6
#define K_SETUP -7
#define K_MODULES -8
#define K_SENSORS -9
#define K_ALARMS -10
#define K_ALARM -11
#define K_DEBUG -12
#define K_MAIN -13
#define K_ID -14
#define K_TYPE -15
#define K_ACTIVE -16
#define K_MIN -17
#define K_MAX -18
#define K_MINUS -19
#define K_CLEAR -20
#define K_ICON -21
#define K_RANDOM -22
#define K_SAVE -23
#define K_EXIT -24
#define K_PAGE -25
#define K_MODEM -26
#define K_TEST -27
#define K_VERSION -28
#define K_SOUND -29
#define K_SMS -30
#define K_RAPORT -31
#define K_NAME -32
#define K_DEFAULT -33
#define K_RESET -34
#define K_NET -35
#define K_ON -36
#define K_OFF -37
#define K_1H -38
#define K_24H -39
#define K_DATETIME -40
#define K_OUT -41
#define K_UP -42
#define K_DOWN -43
#define K_DAY_UP -60
#define K_DAY_DO -61
#define K_MONTH_UP -62
#define K_MONTH_DO -63
#define K_YEAR_UP -64
#define K_YEAR_DO -65
#define K_HOUR_UP -66
#define K_HOUR_DO -67
#define K_MINUTE_UP -68
#define K_MINUTE_DO -69
#define K_SECOND_UP -70
#define K_SECOND_DO -71
#define K_NOW -72
#define K_NONE -999

// ============ GSM ============
#define G_CONNECT 1

// ============ SERIAL BAUD RATES ============
#define SERIAL_BAUD 115200
#define MODEM_BAUD 115200
#define MP3_BAUD 9600

// ============ LIMITS ============
#define MAX_INPUT_LEN 20
#define MAX_MOBILE_LEN 11
#define MAX_SENSOR_NAME 11
#define MAX_SMS_LEN 300

#endif // CONFIG_H
