package com.ecowater;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.lang.reflect.Method;
import java.util.ArrayList;

public class BluetoothDevicesActivity extends AppCompatActivity {
    private ListView aListView;
    private BluetoothDeviceListAdapter anAdapter;
    private ArrayList<BluetoothDevice> aDeviceList;
    private int aPositionReference;
    private ProgressBar progressBar;

    // Bluetooth
    private final BroadcastReceiver aReceiver = new BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            switch (action) {
                case BluetoothDevice.ACTION_BOND_STATE_CHANGED:
                    final int prevState = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR);
                    final int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);

                    if(prevState == BluetoothDevice.BOND_BONDING) {
                        progressBar.setVisibility(View.INVISIBLE);
                        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                    }

                    if (prevState == BluetoothDevice.BOND_BONDING && state == BluetoothDevice.BOND_BONDED) {
                        showToast(getString(R.string.bonded));
                        progressBar.setVisibility(View.INVISIBLE);
                        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);

                        BluetoothDevice aBondedDevice = (BluetoothDevice) anAdapter.getItem(aPositionReference);
                        String deviceMacAddress = aBondedDevice.getAddress();
                        System.out.println(deviceMacAddress);
                        if (deviceMacAddress.equals(Constants.HC06_MAC_ADDRESS)) {
                            Intent arduinoIntent = new Intent(BluetoothDevicesActivity.this, ArduinoActivity.class);
                            arduinoIntent.putExtra("HC06_Mac_Address", deviceMacAddress);
                            startActivity(arduinoIntent);
                        }

                    } else if (prevState == BluetoothDevice.BOND_BONDED && state == BluetoothDevice.BOND_NONE) {
                        showToast(getString(R.string.not_bonded));
                        progressBar.setVisibility(View.INVISIBLE);
                        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                    }

                    anAdapter.notifyDataSetChanged();
                    break;

            }
        }
    };
    private BluetoothDeviceListAdapter.ButtonOnClickListener pairDeviceBtnListener = new BluetoothDeviceListAdapter.ButtonOnClickListener() {
        @SuppressLint("MissingPermission")
        @Override
        public void onButtonClick(int position) {
            BluetoothDevice device = aDeviceList.get(position);
            progressBar.setVisibility(View.VISIBLE);
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);

            if (device.getBondState() == BluetoothDevice.BOND_BONDED) {
                unpairDevice(device);
            } else {
                showToast(getString(R.string.pairing));
                aPositionReference = position;
                pairDevice(device);

            }
        }
    };

    // Lifecycle
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bluetooth_devices);
        setTitle(getString(R.string.bt_devices));

        aListView = (ListView) findViewById(R.id.found_devices_list);
        progressBar = (ProgressBar) findViewById(R.id.progressBar2);

        progressBar.setVisibility(View.INVISIBLE);
        aDeviceList = getIntent().getExtras().getParcelableArrayList("device.list");

        anAdapter = new BluetoothDeviceListAdapter(this);
        anAdapter.setData(aDeviceList);

        anAdapter.setListener(pairDeviceBtnListener);
        aListView.setAdapter(anAdapter);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        registerReceiver(aReceiver, filter);

    }

    @Override
    public void onDestroy() {
        unregisterReceiver(aReceiver);

        super.onDestroy();
    }

    // Bluetooth utils
    private void pairDevice(BluetoothDevice device) {
        try {
            Method method = device.getClass().getMethod("createBond", (Class[]) null);
            method.invoke(device, (Object[]) null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void unpairDevice(BluetoothDevice device) {
        try {
            Method method = device.getClass().getMethod("removeBond", (Class[]) null);
            method.invoke(device, (Object[]) null);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Layout utils
    private void showToast(String message) {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }
}

