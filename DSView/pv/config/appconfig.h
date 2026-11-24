/*
 * This file is part of the DSView project.
 * DSView is based on PulseView.
 * 
 * Copyright (C) 2021 DreamSourceLab <support@dreamsourcelab.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <string>
#include <vector>
#include <QString>
#include <QByteArray>
#include <QColor>

#define LAN_CN  25
#define LAN_EN  31
#define LAN_DE  7

#define THEME_STYLE_DARK   "dark"
#define THEME_STYLE_LIGHT  "light"
#define THEME_STYLE_LATTE  "latte"
#define THEME_STYLE_FRAPPE "frappe"

#define APP_NAME  "DSView"

#define RULER_UNIT_SAMPLES "Samples"
#define RULER_UNIT_TIME    "Time"

// Range of AppOptions::toolbarIconSize; 0 means the style's default.
#define MinToolbarIconSize     16
#define MaxToolbarIconSize     64
#define DefaultToolbarIconSize 32

//--------------------api---
QString GetIconPath();
QString GetAppDataDir();
QString GetFirmwareDir();
QString GetUserDataDir();
QString GetDecodeScriptDir();
QString GetProfileDir();

//------------------class
  
class StringPair
{
public:
   StringPair(const std::string &key, const std::string &value);
   std::string m_key;
   std::string m_value;
};


#define APP_CONFIG_VERSION  3
#define NO_POINT_VALUE  -10000

// Default colours for logic trace levels.
#define DEFAULT_LOGIC_HIGH_COLOR  QColor(0x00, 0xC0, 0x00)
#define DEFAULT_LOGIC_LOW_COLOR   QColor(0x7E, 0x7E, 0x7E)
#define DEFAULT_LOGIC_FILL_COLOR  QColor(0x3B, 0x3B, 0x3C, 0x78)

// Defaults for a new configuration.
#define DEFAULT_FONT_SIZE           11.0f
#define DEFAULT_TRACE_HEIGHT_FACTOR 0.54166179895401f

struct AppOptions
{   
    int   version;
    bool  quickScroll;
    bool  warnofMultiTrig;
    bool  originalData;
    bool  ableSaveLog;
    bool  appendLogMode;
    int   logLevel;
    bool  transDecoderDlg;
    bool  trigPosDisplayInMid;
    bool  displayProfileInBar;
    bool  swapBackBufferAlways;
    bool  autoScrollLatestData;
    bool  verticalScrollIsZoom;
    float fontSize;
    float traceHeightFactor;
    QString rulerTimeUnits;
    bool antialias;
    int minDecoderFontWidthPercent;
    int maxDecoderFontWidthPercent;
    bool decoderDynamicFontWidth;
    bool dontAskSaveOnExit;
    float logicSignalLineWidth;
    bool logicChannelDivider;
    bool dsoSplitChannels;
    float dsoSignalLineWidth;
    int   toolbarIconSize; // in px, 0 = the style's default
    int dsoPersistenceMs; // 0 = off; fade-out time of previous scope frames
    bool  colorLogicHigh;     // Draw HIGH segments of logic traces in logicHighColor
    bool  colorLogicLow;      // Draw LOW segments of logic traces in logicLowColor
    QColor logicHighColor;    // HIGH segment colour (colorLogicHigh)
    QColor logicLowColor;     // LOW segment colour (colorLogicLow)
    bool  fillLogicHigh;      // Shade the area under HIGH pulses
    QColor logicFillColor;    // Shade colour, with alpha (fillLogicHigh)
    bool  showSamplePoints;   // Mark each sample with a dot when zoomed in
    bool  hoverLine;          // Vertical line following the mouse across all traces
    bool  cursorDeltaBand;    // Shade cursor pairs (1-2, 3-4, ...) and show the time between them
    bool  triggerLine;        // Clearly visible line at the trigger position
    bool  highlightHoverRow;  // Highlight the channel row under the mouse
    bool  decoderChannelColours; // Bus channels of a decoder in its decoded colours
    bool  floatingMeasure;    // Measure panel: "Enable floating measurement" (brackets and values at the mouse)
    bool  showTimeBar;        // Logic: thin bar above the ruler standing for the whole capture
    std::vector<StringPair> m_protocolFormats;
};
 
 // The dock pannel open status.
 struct DockOptions
 {
  bool        decodeDock;
  bool        triggerDock;
  bool        measureDock;
  bool        searchDock;
};

struct FrameOptions
{ 
  QString     style;
  int         language; 
  int         left; //frame region
  int         top;
  int         right;
  int         bottom;
  int         x;
  int         y;
  int         ox;
  int         oy;
  bool        isMax;
  QString     displayName;
  QByteArray  windowState;

  DockOptions   _logicDock;
  DockOptions   _analogDock;
  DockOptions   _dsoDock;
};

struct UserHistory
{ 
  QString   exportDir;
  QString   saveDir;
  bool      showDocuments;
  QString   screenShotPath;
  QString   sessionDir;
  QString   openDir;
  QString   protocolExportPath;
  QString   exportFormat;
  QString   protocolExportSeparator; // Column separator for decoder (protocol) export
  bool      showDriverHint;
};

struct FontParam
{
  QString   name;
  float     size;
};

struct FontOptions
{
  FontParam toolbar;
  FontParam channelLabel;
  FontParam channelBody;
  FontParam ruler;
  FontParam title;
  FontParam other;
};

class AppConfig
{
private:
  AppConfig();
  ~AppConfig();
  AppConfig(AppConfig &o);

public:
  static AppConfig &Instance();

  void LoadAll();
  void SaveApp();  
  void SaveHistory();
  void SaveFrame();
  
  void SetProtocolFormat(const std::string &protocolName, const std::string &value);
  std::string GetProtocolFormat(const std::string &protocolName); 

  inline bool IsLangCn()
  {
    return frameOptions.language == LAN_CN;
  }

  static void GetFontSizeRange(float *minSize, float *maxSize);

  // The configured application font size, clamped to the valid range. Used
  // for trace-area text so it honours the user's font-size setting.
  float GetTraceFontSize();

  bool IsDarkStyle();

  QColor GetStyleColor();

public:
  AppOptions    appOptions;
  UserHistory   userHistory;
  FrameOptions  frameOptions;
};
