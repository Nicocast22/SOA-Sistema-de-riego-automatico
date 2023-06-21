package com.ecowater;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.widget.ListView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.lang.reflect.Method;
import java.util.ArrayList;

public class BluetoothDevicesActivity extends AppCompatActivity {
    private ListView aListView;
    private BluetoothDeviceListAdapter mAdapter;
    private ArrayList<BluetoothDevice> aDeviceList;
    private int aPositionReference;

    // Bluetooth
    private final BroadcastReceiver aReceiver = new BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            switch (action) {
                case BluetoothDevice.ACTION_BOND_STATE_CHANGED:
                    final int prevState = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR);
                    final int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);

                    if (state == BluetoothDevice.BOND_BONDED && prevState == BluetoothDevice.BOND_BONDING) {
                        showToast("Emparejado");
                        BluetoothDevice dispositivo = (BluetoothDevice) mAdapter.getItem(aPositionReference);

                        //se inicia el Activity de comunicacion con el bluethoot, para transferir los datos.
                        //Para eso se le envia como parametro la direccion(MAC) del bluethoot Arduino
                        String direccionBluethoot = dispositivo.getAddress();
//                    Intent i = new Intent(DeviceListActivity.this, activity_comunicacion.class);
//                    i.putExtra("Direccion_Bluethoot", direccionBluethoot);
//
//                    startActivity(i);

                    } else if (state == BluetoothDevice.BOND_NONE && prevState == BluetoothDevice.BOND_BONDED) {
                        showToast("No emparejado");
                    }

                    mAdapter.notifyDataSetChanged();
                    break;

            }
        }
    };
    private BluetoothDeviceListAdapter.ButtonOnClickListener pairDeviceBtnListener = new BluetoothDeviceListAdapter.ButtonOnClickListener() {
        @SuppressLint("MissingPermission")
        @Override
        public void onButtonClick(int position) {
            BluetoothDevice device = aDeviceList.get(position);

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

        aDeviceList = getIntent().getExtras().getParcelableArrayList("device.list");

        mAdapter = new BluetoothDeviceListAdapter(this);
        mAdapter.setData(aDeviceList);

        mAdapter.setListener(pairDeviceBtnListener);
        aListView.setAdapter(mAdapter);

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

