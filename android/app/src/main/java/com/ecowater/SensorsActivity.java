package com.ecowater;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
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
import androidx.core.content.ContextCompat;

public class SensorsActivity extends AppCompatActivity implements SensorEventListener {

    // Light sensor
    private final int LUX_THRESHOLD = 1;
    // Flashlight
    private final boolean FLASHLIGHT_ON = true;
    private final boolean FLASHLIGHT_OFF = false;
    // Sensor
    private SensorManager sensorManager;
    private Sensor mLight;
    // Camera
    private CameraManager cameraManager;
    private String cameraId;
    private boolean isFlashlightOn = false;

    // Layout
    private TextView onText;
    private TextView offText;
    private TextView luxValueText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sensors);

        onText = findViewById(R.id.flashOnText);
        offText = findViewById(R.id.flashOffText);
        luxValueText = findViewById(R.id.lightSensorValue);

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
        sensorsCleanUp();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        sensorsCleanUp();
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onSensorChanged(SensorEvent event) {
        float lux = event.values[0];

        luxValueText.setText(Float.toString(lux));

        if (lux <= LUX_THRESHOLD && !isFlashlightOn) {
            toggleFlashlight(FLASHLIGHT_ON);

            isFlashlightOn = FLASHLIGHT_ON;
            showToast(getString(R.string.flash_on));
            updateTextStyles(isFlashlightOn);
        }

        if (lux > LUX_THRESHOLD && isFlashlightOn) {
            toggleFlashlight(FLASHLIGHT_OFF);

            isFlashlightOn = FLASHLIGHT_OFF;
            showToast(getString(R.string.flash_off));
            updateTextStyles(isFlashlightOn);

        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {

    }

    private void initializeCamera() {
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
        alert.setTitle(getString(R.string.oops));
        alert.setMessage(getString(R.string.flash_not_available_on_device));
        alert.setButton(DialogInterface.BUTTON_POSITIVE, "OK", (dialog, which) -> finish());
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
        Toast.makeText(SensorsActivity.this, text, Toast.LENGTH_SHORT).show();
    }

    private void updateTextStyles(boolean flashlightStatus) {
        if (flashlightStatus) {
            onText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.semanticSuccess));
            offText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.disabledError));
        } else {
            onText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.disabledSuccess));
            offText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.semanticError));
        }
    }

    private void sensorsCleanUp() {
        if (isFlashlightOn) {
            toggleFlashlight(FLASHLIGHT_OFF);
        }
        sensorManager.unregisterListener(this);
    }
}