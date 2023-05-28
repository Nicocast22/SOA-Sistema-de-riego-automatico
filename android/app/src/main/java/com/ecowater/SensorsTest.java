package com.ecowater;

import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

public class SensorsTest extends AppCompatActivity implements SensorEventListener {

    // Sensors
    private SensorManager sensorManager;
    private Sensor mLight;

    // Camera
    private CameraManager cameraManager;
    private String cameraId;

    // Light sensor
    private final int LUX_THRESHOLD = 1;

    // Flashlight
    private final boolean FLASHLIGHT_ON = true;
    private final boolean FLASHLIGHT_OFF = false;
    private boolean isFlashlightOn = false;

    // Layout
    private TextView onText;
    private TextView offText;
    private TextView luxValueText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sensors_test);

        onText = findViewById(R.id.textView6);
        offText = findViewById(R.id.textView7);
        luxValueText = findViewById(R.id.textView5);

        initializeCamera();
        updateTextStyles(isFlashlightOn);
    }

    @Override
    protected void onResume() {
        super.onResume();
        sensorManager.registerListener(this, mLight, SensorManager.SENSOR_DELAY_NORMAL);
    }

    @Override
    protected void onPause() {
        super.onPause();
        sensorManager.unregisterListener(this);
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        Float lux = event.values[0];

        luxValueText.setText(lux.toString());

        if (lux <= LUX_THRESHOLD && !isFlashlightOn) {
            toggleFlashlight(FLASHLIGHT_ON);

            isFlashlightOn = FLASHLIGHT_ON;
            showToast("Flash ON");
            updateTextStyles(isFlashlightOn);
        }

        if (lux > LUX_THRESHOLD && isFlashlightOn) {
            toggleFlashlight(FLASHLIGHT_OFF);

            isFlashlightOn = FLASHLIGHT_OFF;
            showToast("Flash OFF");
            updateTextStyles(isFlashlightOn);

        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {

    }

    private final void initializeCamera() {
        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        mLight = sensorManager.getDefaultSensor(Sensor.TYPE_LIGHT);

        boolean isFlashlightAvailable = getApplicationContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY) && getApplicationContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_FLASH);
        if (!isFlashlightAvailable) {
            showFlashError();
        }

        cameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            cameraId = cameraManager.getCameraIdList()[0];
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void showFlashError() {
        AlertDialog alert = new AlertDialog.Builder(this).create();
        alert.setTitle("Oops!");
        alert.setMessage("Flash not available in this device...");
        alert.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                finish();
            }
        });
        alert.show();
    }

    private void toggleFlashlight(boolean status) {
        try {
            cameraManager.setTorchMode(cameraId, status);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void showToast(String text) {
        Toast.makeText(SensorsTest.this, text, Toast.LENGTH_SHORT).show();
    }

    private void updateTextStyles(boolean flashlightStatus) {
        if (flashlightStatus) {
            onText.setBackgroundColor(Color.parseColor("#32C84A"));
            offText.setBackgroundColor(Color.parseColor("#D1A3A3"));
        } else {
            onText.setBackgroundColor(Color.parseColor("#B1D0B6"));
            offText.setBackgroundColor(Color.parseColor("#C83232"));
        }
    }
}