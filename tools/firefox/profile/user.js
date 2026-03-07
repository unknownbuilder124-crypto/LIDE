// LIDE Firefox Profile Configuration

// Set LIDE as the default theme
user_pref("lightweightThemes.selectedThemeID", "firefox-compact-dark@mozilla.org");
user_pref("lightweightThemes.isTheme", true);
user_pref("ui.systemUsesDarkTheme", 1);

// Home page
user_pref("browser.startup.homepage", "https://www.google.com");
user_pref("browser.startup.page", 1);

// Search engine
user_pref("browser.search.defaultenginename", "Google");
user_pref("browser.search.selectedEngine", "Google");

// Disable telemetry
user_pref("toolkit.telemetry.enabled", false);
user_pref("toolkit.telemetry.rejected", true);
user_pref("datareporting.healthreport.uploadEnabled", false);

// Privacy settings
user_pref("privacy.donottrackheader.enabled", true);
user_pref("privacy.trackingprotection.enabled", true);
user_pref("privacy.trackingprotection.pbmode.enabled", true);

// Download settings
user_pref("browser.download.useDownloadDir", true);
user_pref("browser.download.folderList", 1);
user_pref("browser.download.manager.showWhenStarting", true);

// LIDE integration
user_pref("browser.tabs.drawInTitlebar", false); // Let LIDE handle titlebar
user_pref("browser.uidensity", 1); // Compact mode