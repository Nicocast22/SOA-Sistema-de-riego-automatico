package com.ecowater;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.widget.Button;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.util.Set;

public class MainActivity extends AppCompatActivity {

    private Button btBtn;
    private Button arduinoBtn;
    private Button sensorsBtn;
    private BluetoothAdapter aBluetoothAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Ocultamos la actionBar en la mainScreen
        getSupportActionBar().hide();

        // Si no soportamos Bluetooth, finalizamos la activity y con ello la app.
        // Este check se realiza en BluetoothActivity tambien, siendo redundante.
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)) {
            Toast.makeText(this, "Bluetooth no soportado", Toast.LENGTH_SHORT).show();
            finish();
        }

        aBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        btBtn = (Button) findViewById(R.id.bluetoothBtn);
        arduinoBtn = (Button) findViewById(R.id.arduinoBtn);
        sensorsBtn = (Button) findViewById(R.id.sensorsBtn);

        // Injectamos la app en el Singleton
        ReportAppInsights.appToReport = getApplication();
    }

    @Override
    protected void onStart() {
        super.onStart();

        btBtn.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, BluetoothActivity.class)));
        arduinoBtn.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, ArduinoActivity.class)));
        sensorsBtn.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, SensorsActivity.class)));

        checkConnectionWithArduino();
    }

    // Comprobamos si el dispositivo esta vinculado al HC06, para habilitar el boton de la activity correspondiente
    private void checkConnectionWithArduino() {
        @SuppressLint("MissingPermission") Set<BluetoothDevice> pairedDevices = aBluetoothAdapter.getBondedDevices();

        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                String deviceAddress = device.getAddress();
                if (deviceAddress.equals(Constants.HC06_MAC_ADDRESS)) {
                    arduinoBtn.setEnabled(true);
                }
            }
        } else {
            arduinoBtn.setEnabled(false);
        }
    }
}
