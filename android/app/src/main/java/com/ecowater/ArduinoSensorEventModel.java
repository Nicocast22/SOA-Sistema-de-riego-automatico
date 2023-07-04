package com.ecowater;

import java.util.HashMap;

public class ArduinoSensorEventModel {

    private Float aLightSensorValue;
    private Float aWaterLevelSensorValue;

    public ArduinoSensorEventModel(float lightSensorValue, float waterLevelSensorValue) {
        aLightSensorValue = Float.valueOf(lightSensorValue);
        aWaterLevelSensorValue = Float.valueOf(waterLevelSensorValue);
    }

    public HashMap<String, String> asHashMap(){
        HashMap<String, String> map = new HashMap<String, String>();
        map.put("LightSensor", aLightSensorValue.toString());
        map.put("WaterSensor", aWaterLevelSensorValue.toString());
        return map;
    }
}
