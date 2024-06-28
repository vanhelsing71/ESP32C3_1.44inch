//屏幕驱动库
#include <TFT_eSPI.h>
//spi驱动库
#include <SPI.h>
//图片解码库
#include <TJpg_Decoder.h>
//json库,用于网页解析
#include "ArduinoJson.h"
//时间库,用于时钟部分
#include <ezTime.h>
//wifi库
#include <WiFi.h>

//http请求库
#include <HTTPClient.h>
//创建wifi的udp服务的库
//#include <WiFiUdp.h>
//Static storage area, used to store some fixed parameters
#include <EEPROM.h>

#include <Arduino.h>
#include "lvgl.h"
//天气图标
#include "weathernum.h"

//温湿度图标
#include "img/temperature.h"
#include "img/humidity.h"

//字体文件用于时钟显示
#include "font/FxLED_32.h"
//字体文件用于星期和日月,粉丝数显示
#include "font/zkyyt12.h"
//字体文件用于城市名字显示
#include "font/city10.h"
//字体文件用于天气显示
#include "font/tq10.h"
//#include "font/AAA.h"
#include "font/ALBB10.h"
#include "font/font_20.h"

//此处添加WIFI信息或者个人热点WLAN信息
const char *ssid     = "spotpear";  //Wifi名字 
const char *password = "12345678"; //Wifi密码 

//const char *ssid     = "spotpear";  //Wifi名字
//const char *password = "12345678"; //Wifi密码


WeatherNum wrat;  //天气对象
int prevTime = 0;
int AprevTime = 0;
int Anim = 0;
uint32_t targetTime = 0;


//背景颜色
uint16_t bgColor = 0x0000;

//字体颜色

//时间小时分钟字体颜色
uint16_t timehmfontColor = 0x0F0F;
//时间秒字体颜色
uint16_t timesfontColor = 0x0F0F;
//星期字体颜色
uint16_t weekfontColor = 0xFFFF;
//日月字体颜色
uint16_t monthfontColor = 0xFFFF;
//温湿度字体颜色
uint16_t thfontColor = 0xFFFF;
//左上角轮播字体颜色
uint16_t tipfontColor = 0xFFFF;
//城市名称字体颜色
uint16_t cityfontColor = 0xFFFF;
//空气质量字体颜色
uint16_t airfontColor = 0xFFFF;
//b站粉丝数字体颜色
uint16_t bilifontColor = 0xF81F;




//线框颜色

uint16_t xkColor = 0xFFFF;

//lvgl
static const uint16_t screenWidth = 128;
static const uint16_t screenHeight = 128;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenHeight * screenWidth / 10];


static String cityCode = "Napoli";                   //If you need to add city information manually, fill in the name of the city in "", and then comment out the assignment in the function getCityCode
char *api_key = "fbf5a0e942e6fea3ff18103b9fd46ed9";  //API 密钥


//WiFiUDP Udp;
//unsigned int localPort = 8000;
WiFiClient wificlient;
float timeZone;     //时区

//time_t getNtpTime();
//time_t getTimeWorldApi();
void digitalClockDisplay();
void printDigits(int digits);
String num2str(int digits);
//void sendNTPpacket(IPAddress &address);

//byte setNTPSyncTime = 20; //Set NTP time synchronization frequency, synchronize once every 10 minutes

int backLight_hour = 0;

time_t prevDisplay = 0;  // display time


//-----------------------------------------

//----------------------Temperature and humidity and other parameters------------------
unsigned long wdsdTime = 0;
byte wdsdValue = 0;
String wendu1 = "", wendu2 = "", shidu = "", yaqiang = "", tianqi = "", kjd = "";

//----------------------------------------------------

//LV_IMG_DECLARE(TKR_A);
//static lv_obj_t *logo_imga = NULL;


TFT_eSPI tft = TFT_eSPI(128, 128);  // Please configure the pins yourself in the User_Setup.h file in the tft_espi library.
TFT_eSprite clk = TFT_eSprite(&tft);



bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {

  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  // Return 1 to decode next block
  return 1;
}


void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}


void tkr(void) {
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenHeight * screenWidth / 10);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = 128;
  disp_drv.ver_res = 128;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_color(&style, lv_color_black());
  lv_obj_add_style(lv_scr_act(), &style, 0);

  // we don't want the GIF
  //logo_imga = lv_gif_create(lv_scr_act());
  //lv_obj_center(logo_imga);
  //lv_obj_align(logo_imga, LV_ALIGN_LEFT_MID, 50, 0);
  //lv_gif_set_src(logo_imga, &TKR_A);

  lv_timer_handler();
  delay(1);
}


//-----------------------------------City code acquisition and display
//Get city information ID based from the IP address
void getCityCode() {

  String URL = "http://ip-api.com/json/?fields=city,lat,lon";

  HTTPClient httpClient;
  httpClient.begin(wificlient, URL);


  //Start a connection and send an HTTP request
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);

  if (httpCode == HTTP_CODE_OK) {

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, httpClient.getString());
    String city = doc["city"];
    Serial.println("Detected city: " + city);
    //cityCode=city; //Get local city ID commented because we have hardcoded the city
  }

  httpClient.end();
}



void getCityTime() {
  //getCityCode(); //we don't need this
  String URL = "http://api.openweathermap.org/data/2.5/weather?q=" + cityCode + "&appid=" + String(api_key) + "&units=metric";  //524901
  HTTPClient httpClient;
  httpClient.begin(URL);

  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + JSON_OBJECT_SIZE(40) + 470;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, httpClient.getString());

    long timezone1 = doc["timezone"];  //获取时区信息
    timeZone = (float)(timezone1 / 3600);

    Serial.println("Obtained city information successfully");
  } else {
    Serial.println("Failed to obtain city information");
    Serial.print(httpCode);
  }
  httpClient.end();
}

int temp_i1, temp_i2;
String scrollText[6];
// Get city weather fc_24_en  1694222530090  401070101 fbf5a0e942e6fea3ff18103b9fd46ed9
void getCityWeater() {

  float temp_f, temp_min_f, temp_max_f;
  getCityCode();
  String URL = "http://api.openweathermap.org/data/2.5/weather?q=" + cityCode + "&appid=" + String(api_key) + "&units=metric&lang=it";  //524901

  //创建 HTTPClient 对象
  HTTPClient httpClient;
  httpClient.begin(URL);


  int httpCode = httpClient.GET();
  Serial.println("Getting weather data");
  Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {

    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + JSON_OBJECT_SIZE(40) + 470;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, httpClient.getString());
    // JsonObject sk = doc.as<JsonObject>();
    // String str1 = httpClient.getString();

    float temp = doc["main"]["temp"];        // Temperature in Celsius
    int humidity = doc["main"]["humidity"];  // Humidity in percentage
    int pressure = doc["main"]["pressure"];
    String description = doc["weather"][0]["description"];  // Weather description 天气
    String icon = doc["weather"][0]["icon"];
    float temp_min = doc["main"]["temp_min"];
    float temp_max = doc["main"]["temp_max"];
    float temp_perc = doc["main"]["feels_like"];
    int visibility = doc["visibility"];

    temp_f = 32 + temp * 1.8;
    temp_min_f = 32 + temp_min * 1.8;
    temp_max_f = 32 + temp_max * 1.8;

    temp_i1 = int(temp_f);
    temp_i2 = int(temp);

    wendu1 = String(temp_i1);         //温度 ℉
    wendu2 = String(temp_i2);         //温度 ℃
    shidu = String(humidity);         //湿度
    yaqiang = String(pressure);       //压强
    tianqi = String(description);     //天气
    kjd = String(visibility / 1000);  //可见度

    clk.setColorDepth(8);
    clk.loadFont(ALBB10);

    // shidu = sk["SD"].as<String>();

    //城市名称
    clk.createSprite(77, 16);
    clk.fillSprite(bgColor);
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(cityfontColor, bgColor);
    clk.drawString(cityCode, 1, 8);  //
    clk.pushSprite(1, 89);
    clk.deleteSprite();
    clk.unloadFont();
    // temp=26;
    uint16_t pm25BgColor = tft.color565(156, 202, 127);
    if (temp < 10)
      pm25BgColor = tft.color565(0, 0, 255);  //蓝色
    else if (temp < 20 && temp >= 10)
      pm25BgColor = tft.color565(46, 185, 201);  //淡蓝色
    else if (temp >= 20 && temp <= 25)
      pm25BgColor = tft.color565(156, 202, 127);  //绿色
    else if (temp > 25 && temp < 30)
      pm25BgColor = tft.color565(222, 202, 24);  //黄色
    else if (temp >= 30)
      pm25BgColor = tft.color565(136, 11, 32);  //红色

    clk.setColorDepth(8);
    clk.loadFont(ALBB10);
    clk.createSprite(36, 15);
    clk.fillSprite(bgColor);
    clk.fillRoundRect(0, 0, 32, 15, 4, pm25BgColor);
    clk.setTextDatum(ML_DATUM);
    clk.setTextColor(airfontColor);
    clk.drawString("Temp", 3, 7);

    clk.pushSprite(93, 69);
    clk.deleteSprite();
    clk.unloadFont();

    scrollText[0] = "T Min: " + String(temp_min) + "℃";
    scrollText[1] = "T Max: " + String(temp_max) + "℃";
    scrollText[2] = "Percepita: " + String(temp_perc) + "℃";
    scrollText[3] = "Meteo: " + String(tianqi);
    scrollText[4] = "Pressione: " + String(yaqiang) + "hPa";
    scrollText[5] = "Visibilita': " + String(kjd) + "km";
    wrat.printfweather1(1, 47, icon);

    Serial.println("get success");

  } else {
    Serial.println("Request city weather error：");
    Serial.print(httpCode);
  }

  //关闭ESP32与服务器连接
  httpClient.end();
}

//---------------- Temperature and humidity carousel display ------------------------------------------

void weatherWarning() {  //Switches to display temperature and humidity every 5 seconds. This data is the outdoor parameter obtained by the weather station.
  if (millis() - wdsdTime > 5000) {
    wdsdValue = wdsdValue + 1;
    //Serial.println("wdsdValue0" + String(wdsdValue));
    clk.setColorDepth(8);
    clk.loadFont(ALBB10);
    switch (wdsdValue) {
      case 1:
        //Serial.println("wdsdValue1" + String(wdsdValue));
        TJpgDec.drawJpg(82, 89, temperature, sizeof(temperature));  //temperature icon
        for (int i = 20; i > 0; i--) {
          clk.createSprite(30, 16);
          clk.fillSprite(bgColor);
          clk.setTextDatum(ML_DATUM);
          clk.setTextColor(thfontColor, bgColor);
          clk.drawString(wendu2 + "℃", 1, i + 8);  //AW wendu+
          clk.pushSprite(98, 89);
          clk.deleteSprite();
          delay(10);
        }
        break;
      case 2:
        //Serial.println("wdsdValue2" + String(wdsdValue));
        TJpgDec.drawJpg(82, 89, humidity, sizeof(humidity));  //Humidity icon
        for (int i = 20; i > 0; i--) {
          clk.createSprite(30, 16);
          clk.fillSprite(bgColor);
          clk.setTextDatum(ML_DATUM);
          clk.setTextColor(thfontColor, bgColor);
          clk.drawString(shidu + "%", 1, i + 8);
          clk.pushSprite(98, 89);
          clk.deleteSprite();
          delay(10);
        }
        wdsdValue = 0;
        break;
    }
    wdsdTime = millis();
    clk.unloadFont();
  }
}


//----------------------------左上角天气信息轮播-------------------------
int currentIndex = 0;

TFT_eSprite clkb = TFT_eSprite(&tft);

void scrollBanner() {
  if (millis() - prevTime > 3500) {  //3.5秒切换一次

    if (scrollText[currentIndex]) {

      clkb.setColorDepth(8);
      clkb.loadFont(ALBB10);

      for (int pos = 20; pos > 0; pos--) {
        scrollTxt(pos);
      }

      clkb.deleteSprite();
      clkb.unloadFont();

      if (currentIndex >= 5) {
        currentIndex = 0;  //回第一个
      } else {
        currentIndex += 1;  //准备切换到下一个
      }
    }
    prevTime = millis();
  }
}

void scrollTxt(int pos) {
  clkb.createSprite(128, 16);
  clkb.fillSprite(bgColor);
  clkb.setTextWrap(false);
  clkb.setTextDatum(ML_DATUM);
  clkb.setTextColor(tipfontColor, bgColor);
  clkb.drawString(scrollText[currentIndex], 1, pos + 8);
  clkb.pushSprite(1, 1);
}

//----------------------------------------------




byte loadNum = 6;
void loading(byte delayTime)  //绘制进度条
{
  clk.setColorDepth(8);
  clk.createSprite(100, 100);  //创建窗口
  clk.fillSprite(0x0000);      //填充率

  clk.drawRoundRect(0, 0, 100, 16, 8, 0xFFFF);      //空心圆角矩形
  clk.fillRoundRect(3, 3, loadNum, 10, 5, 0xFFFF);  //实心圆角矩形
  clk.setTextDatum(CC_DATUM);                       //设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("Getting WiFi...", 50, 40, 2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawRightString("MiNiApp V1.0", 100, 60, 2);
  clk.pushSprite(14, 40);  //窗口位置

  //clk.setTextDatum(CC_DATUM);
  //clk.setTextColor(TFT_WHITE, 0x0000);
  //clk.pushSprite(130,180);

  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}


void get_wifi() {
  //Serial.begin(9600); commented because we already did that in setup
  // 连接网络
  WiFi.begin(ssid, password);
  //等待wifi连接
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  //连接成功
  Serial.print("IP address: ");      //打印IP地址
  Serial.println(WiFi.localIP());
}

Timezone Italy;

//---------------------------------------------------------------------------
void setup() {

  tkr();
  Serial.begin(9600);
  EEPROM.begin(1024);

  //while (!Serial)  // Wait for the serial connection to be establised.
  //  delay(50);


  tft.begin();           /* TFT init */
  tft.invertDisplay(0);  //Invert all display colors: 1 to invert, 0 to normal
  tft.fillScreen(0x0000);
  tft.setTextColor(TFT_WHITE, 0x0000);
  //Set the rotation angle of the screen display, the parameter is：0, 1, 2, 3
  // Representing 0°、90°、180°、270°
  //Rotate according to actual needs
  tft.setRotation(2);




  tft.setCursor(0, 8, 1);
  // 设置文本颜色为白色，黑色文本背景
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // 设置显示的文字，注意这里有个换行符 \n 产生的效果
  tft.println("---------------------");
  tft.println("Configure mobile WLAN information:");
  tft.println("WIFI name:spotpear");
  tft.println("WIFI password:12345678\n---------------------");
  tft.println("If there is a city information error,execute==>After turning on the phone WLAN,turn off the phone data,and then connect to the router WIFI.");
  tft.println("---------------------");


  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  targetTime = millis() + 1000;
  get_wifi();
  Serial.print("Connecting to WIFI ");
  // Serial.println(ssid);
  //WiFi.begin(ssid,password);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  while (WiFi.status() != WL_CONNECTED) {
    loading(70);

    if (loadNum >= 94) {
      // SmartConfig();
      break;
    }
  }
  
  waitForSync();
  Italy.setLocation("Europe/Rome");
  Serial.println("Rome time: " + Italy.dateTime());
  tft.fillScreen(TFT_BLACK);
  while (loadNum < 94)  //让动画走完
  {
    loading(1);
  }
  loading(1);

  //Udp.begin(localPort);

  getCityTime();  //we need this for knowing the timeZone correction

  //setSyncProvider(getTimeWorldApi);
  //setSyncInterval(setNTPSyncTime*60); //NTP网络同步频率，单位秒。


  //绘制一个视口

  tft.fillScreen(0x0000);
  tft.fillRoundRect(0, 0, 128, 128, 0, bgColor);  //实心矩形


  //绘制线框
  tft.drawFastHLine(0, 0, 128, xkColor);


  tft.drawFastHLine(0, 18, 128, xkColor);  //x:0 y:18 长度为:128
  tft.drawFastHLine(0, 106, 128, xkColor);

  // tft.drawFastVLine(80,0,18,xkColor);

  tft.drawFastHLine(0, 88, 128, xkColor);

  // tft.drawFastVLine(32,88,18,xkColor);
  tft.drawFastVLine(78, 88, 18, xkColor);

  tft.drawFastVLine(60, 106, 20, xkColor);  //line of dayweek 
  tft.drawFastHLine(0, 127, 128, xkColor);

  getCityWeater();
}

unsigned long weaterTime = 0;

void loop() {
  events();

  if (now() != prevDisplay) {
    prevDisplay = now();
    digitalClockDisplay();
  }

  if (millis() - weaterTime > 300000) {  //Weather updated every 5 minutes
    weaterTime = millis();
    getCityWeater();
    // get_Bstation_follow();
    // fanspush();
  }
  digitalClockDisplay();
  scrollBanner();
  weatherWarning();
  lv_timer_handler();
  delay(1);
  //  Serial_set();
}


//时钟显示函数--------------------------------------------------------------------------


void digitalClockDisplay() {

  clk.setColorDepth(8);

  /***Intermediate time zone***/
  //hour and minute
  clk.createSprite(75, 28);
  clk.fillSprite(bgColor);
  clk.loadFont(FxLED_32);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(timehmfontColor, bgColor);
  clk.drawString(hourMinute(), 1, 14, 7);  //绘制时和分
  clk.unloadFont();
  clk.pushSprite(10, 19);
  clk.deleteSprite();

  //秒
  clk.createSprite(50, 28);
  clk.fillSprite(bgColor);

  clk.loadFont(FxLED_32);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(timesfontColor, bgColor);
  clk.drawString(":" + num2str(Italy.second()), 1, 14);

  clk.unloadFont();
  clk.pushSprite(86, 19);
  clk.deleteSprite();
  /***中间时间区***/

  //big temperature
  clk.createSprite(36, 28);
  clk.fillSprite(bgColor);
  clk.loadFont(FxLED_32);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(timehmfontColor, bgColor);
  clk.drawString(wendu2, 1, 14, 7);  //draw big temperature
  clk.unloadFont();
  clk.pushSprite(40, 45);
  clk.deleteSprite();

  clk.createSprite(16, 20);
  clk.fillSprite(bgColor);
  clk.loadFont(ALBB10);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(timehmfontColor, bgColor);
  clk.drawString("℃", 1, 14, 7);
  clk.unloadFont();
  clk.pushSprite(75, 50);
  clk.deleteSprite();



  /***底部***/
  clk.loadFont(ALBB10);
  clk.createSprite(70, 16);
  clk.fillSprite(bgColor);

  //星期
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(weekfontColor, bgColor);
  clk.drawString(week(), 1, 8);
  clk.pushSprite(65, 108);  // 字体的位置 
  clk.deleteSprite();

  // 月日
  clk.createSprite(50, 16);  //box size
  clk.fillSprite(bgColor);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(monthfontColor, bgColor);
  clk.drawString(monthDay(), 1, 8);
  clk.pushSprite(1, 108);
  clk.deleteSprite();

  clk.unloadFont();
  /***底部***/
}

//星期
String week() {
  String wk[7] = { "Domenica", "Lunedi'", "Martedi'", "Mercoledi'", "Giovedi'", "Venerdi'", "Sabato" };
  String s = wk[weekday() - 1];
  return s;
}

//月日
String monthDay() {
  String s = String(day());
  String y = String(year());
  y.remove(0, 2);
  s = s + "/" + month() + "/" + y;
  return s;
}
//时分
String hourMinute() {
  String s = num2str(Italy.hour());
  backLight_hour = s.toInt();
  s = s + ":" + num2str(Italy.minute());
  return s;
}

String num2str(int digits) {
  String s = "";
  delay(9);  //Adjust the speed of time(?)
  if (digits < 10)
    s = s + "0";
  s = s + digits;
  return s;
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
//------------------------------------------------------------------------------------
