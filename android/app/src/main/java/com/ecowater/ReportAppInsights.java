package com.ecowater;

import android.app.Application;

import com.microsoft.appcenter.AppCenter;
import com.microsoft.appcenter.analytics.Analytics;
import com.microsoft.appcenter.crashes.Crashes;

import java.util.HashMap;

public class ReportAppInsights {
    private static ReportAppInsights instance;
    public static Application appToReport;
    private ReportAppInsights()
    {
        AppCenter.start(appToReport, "APPSECRET",
                Analytics.class, Crashes.class);

    }

    public static ReportAppInsights getInstance()
    {
        if(instance == null)
        {
            instance = new ReportAppInsights();
        }
        return instance;
    }

    public void reportEcoWaterEvent(HashMap<String, String> map) {
        Analytics.trackEvent("EcoWaterEvent",map);
    }
}
