package com.ecowater;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

public class ArduinoActivity extends AppCompatActivity
{

    private final StringBuilder aDataString = new StringBuilder();
    private BluetoothAdapter aBluetoothAdapter = null;
    private BluetoothService aBluetoothService = null;
    private StringBuffer anOutStringBuffer;
    private String aConnectedDeviceName = null;
    private Float lightSensorValue;
    private Float waterLevelSensorValue;
    private Button drainageBtn;
    private Button dayStatusBtn;
    private Button waterLevelBtn;
    private TextView dayStatus;
    private TextView tankStatus;
    @SuppressLint("HandlerLeak")
    private final Handler aHandler = new Handler()
    {
        @Override
        public void handleMessage(Message msg)
        {
            switch (msg.what)
            {
                case Constants.MESSAGE_STATE_CHANGE:
                    switch (msg.arg1)
                    {
                        case BluetoothService.STATE_CONNECTED:
                            setStatus("Conectado a" + aConnectedDeviceName);
                            break;
                        case BluetoothService.STATE_CONNECTING:
                            setStatus("Conectando");
                            break;
                        case BluetoothService.STATE_NONE:
                            setStatus("No conectado");
                            break;
                    }
                    break;
                case Constants.MESSAGE_WRITE:
                    // Mensaje enviado al Arduino
                    byte[] writeBuf = (byte[]) msg.obj;
                    String writeMessage = new String(writeBuf);
                    break;
                case Constants.MESSAGE_READ:
                    byte[] readBuf = (byte[]) msg.obj;
                    // Mensaje recibido desde el Arduino

                    String readMessage = new String(readBuf, 0, msg.arg1);
                    aDataString.append(readMessage);

                    // Esperamos a un fin de linea
                    int isEndOfLine = aDataString.indexOf("\r\n");

                    if (isEndOfLine > 0)
                    {
                        // Obtenemos la linea completa y la decodificamos
                        String newString = aDataString.substring(0, isEndOfLine);
                        decodifyMessage(newString);

                        // Limpiamos nuestro "buffer"
                        aDataString.delete(0, aDataString.length());
                    }
                    break;
                case Constants.MESSAGE_DEVICE_NAME:
                    aConnectedDeviceName = msg.getData().getString(Constants.DEVICE_NAME);
                    showToast("Connected to " + aConnectedDeviceName);
                    break;
                case Constants.MESSAGE_TOAST:
                    showToast(msg.getData().getString(Constants.TOAST));
                    break;
            }
        }
    };

    // Lifecycle
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_arduino);
        setTitle(getString(R.string.arduino_interaction));

        assert getSupportActionBar() != null;
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        drainageBtn = (Button) findViewById(R.id.drainageBtn);
        dayStatusBtn = (Button) findViewById(R.id.getDayStatusBtn);
        waterLevelBtn = (Button) findViewById(R.id.getWaterLevelBtn);

        dayStatus = (TextView) findViewById(R.id.dayStatusText);
        tankStatus = (TextView) findViewById(R.id.waterTankStatusText);

        aBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    @Override
    protected void onStart()
    {
        super.onStart();

        Intent intent = getIntent();

        setupBluetooth();
        connectDevice(intent, true);
    }

    @Override
    public void onResume()
    {
        super.onResume();

        if (aBluetoothService != null)
        {
            if (aBluetoothService.getState() == BluetoothService.STATE_NONE)
            {
                aBluetoothService.start();
            }
        }
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        aBluetoothService.stop();
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        aBluetoothService.stop();
    }

    // Utils
    private void showToast(String message)
    {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }

    private void setStatus(CharSequence subTitle)
    {
        final android.app.ActionBar actionBar = this.getActionBar();
        if (null == actionBar)
        {
            return;
        }
        actionBar.setTitle(subTitle);
    }

    public boolean onOptionsItemSelected(MenuItem item)
    {
        finish();
        return true;
    }


    // Bluetooth utils
    private void connectDevice(Intent data, boolean secure)
    {
        Bundle extras = data.getExtras();
        String address = (extras != null) ? extras.getString("HC05_Mac_Address") : Constants.HC05_MAC_ADDRESS;

        BluetoothDevice aBluetoothDevice = aBluetoothAdapter.getRemoteDevice(address);

        if (aBluetoothDevice != null)
        {
            aBluetoothService.connect(aBluetoothDevice, secure);
        }
    }

    private void setupBluetooth()
    {
        drainageBtn.setOnClickListener(v -> sendMessage(Constants.ACTIONS_DRAIN));

        dayStatusBtn.setOnClickListener(v -> sendMessage(Constants.SENSORS_VALUE_LIGHT));

        waterLevelBtn.setOnClickListener(v -> sendMessage(Constants.SENSORS_VALUE_WATER_LEVEL));

        aBluetoothService = new BluetoothService(this.getApplicationContext(), aHandler);
        anOutStringBuffer = new StringBuffer();
    }

    // Stream utils
    private void sendMessage(String message)
    {
        if (aBluetoothService.getState() != BluetoothService.STATE_CONNECTED)
        {
            showToast("No conectado");
            return;
        }

        if (message.length() > 0)
        {
            byte[] send = message.getBytes();
            aBluetoothService.write(send);

            anOutStringBuffer.setLength(0);
        }
    }

    // Logica decodificado
    private void decodifyMessage(String receivedMessage)
    {
        String[] tokens = receivedMessage.split("@");
        switch (tokens[0])
        {
            case "L":
                lightSensorValue = Float.parseFloat(tokens[1]);
                updateLightUI(lightSensorValue);
                break;
            case "W":
                waterLevelSensorValue = Float.parseFloat(tokens[1]);
                updateWaterUI(waterLevelSensorValue);
                break;
        }
    }

    // UI Utils
    private void updateLightUI(Float light)
    {
        if (light == Constants.IS_NIGHTFALL)
        {
            dayStatus.setText("Noche");
            dayStatus.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.night));
        } else
        {
            dayStatus.setText("Dia");
            dayStatus.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.daylight));
        }
    }

    private void updateWaterUI(Float waterLevel)
    {
        if (waterLevel > Constants.THRESHOLD_NO_WATER)
        {
            tankStatus.setText("Sin agua");
            tankStatus.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.disabled));
        } else
        {
            tankStatus.setText("Con agua");
            tankStatus.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.water));
        }
    }
}