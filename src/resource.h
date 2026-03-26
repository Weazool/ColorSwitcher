#pragma once

// Icon
#define IDI_APP_ICON            101

// Tray menu items
#define IDM_PRESET1             1001
#define IDM_PRESET2             1002
#define IDM_DEFAULT             1003
#define IDM_CUSTOMIZE           1004
#define IDM_AUTOSTART           1005
#define IDM_QUIT                1006

// Editor dialog controls
#define IDD_PRESET_EDITOR       2000
#define IDC_PRESET_COMBO        2001
#define IDC_BRIGHTNESS_SLIDER   2002
#define IDC_BRIGHTNESS_LABEL    2003
#define IDC_BRIGHTNESS_VALUE    2004
#define IDC_CONTRAST_SLIDER     2005
#define IDC_CONTRAST_LABEL      2006
#define IDC_CONTRAST_VALUE      2007
#define IDC_GAMMA_SLIDER        2008
#define IDC_GAMMA_LABEL         2009
#define IDC_GAMMA_VALUE         2010
#define IDC_VIBRANCE_SLIDER     2011
#define IDC_VIBRANCE_LABEL      2012
#define IDC_VIBRANCE_VALUE      2013
#define IDC_HUE_SLIDER          2014
#define IDC_HUE_LABEL           2015
#define IDC_HUE_VALUE           2016
// Hotkey controls
#define IDC_HOTKEY_DEFAULT      2017
#define IDC_HOTKEY_PRESET1      2018
#define IDC_HOTKEY_PRESET2      2019
#define IDC_SAVE_BTN            2020
#define IDC_CANCEL_BTN          2021

// Global hotkey IDs (for RegisterHotKey)
#define HOTKEY_ID_DEFAULT       3001
#define HOTKEY_ID_PRESET1       3002
#define HOTKEY_ID_PRESET2       3003

// Custom messages
#define WM_TRAYICON             (WM_USER + 1)
#define WM_HOTKEYS_CHANGED      (WM_USER + 2)
