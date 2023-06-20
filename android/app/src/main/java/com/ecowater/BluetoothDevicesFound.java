package com.ecowater;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import java.lang.reflect.Method;
import java.util.ArrayList;

import android.bluetooth.BluetoothDevice;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.widget.ListView;
import android.widget.Toast;

public class BluetoothDevicesFound extends AppCompatActivity {
    private ListView mListView;
    private DeviceListAdapter mAdapter;
    private ArrayList<BluetoothDevice> aDeviceList;
    private int posicionListBluethoot;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_bluetooth_devices_found);
        mListView = (ListView) findViewById(R.id.found_devices_list);

        aDeviceList = getIntent().getExtras().getParcelableArrayList("device.list");

        mAdapter = new DeviceListAdapter(this);
        mAdapter.setData(aDeviceList);

        mAdapter.setListener(listenerBotonEmparejar);
        mListView.setAdapter(mAdapter);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        registerReceiver(aReceiver, filter);

    }

    @Override
    public void onDestroy() {
        unregisterReceiver(aReceiver);

        super.onDestroy();
    }


    private void showToast(String message) {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }

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

    private DeviceListAdapter.OnPairButtonClickListener listenerBotonEmparejar = new DeviceListAdapter.OnPairButtonClickListener() {
        @SuppressLint("MissingPermission")
        @Override
        public void onPairButtonClick(int position) {
            BluetoothDevice device = aDeviceList.get(position);

            if (device.getBondState() == BluetoothDevice.BOND_BONDED)
            {
                unpairDevice(device);
            }
            else
            {
                showToast("Emparejando");
                posicionListBluethoot = position;
                pairDevice(device);
            }
        }
    };


    private final BroadcastReceiver aReceiver = new BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(action))
            {
                final int prevState = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.ERROR);
                final int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);

                if (state == BluetoothDevice.BOND_BONDED && prevState == BluetoothDevice.BOND_BONDING)
                {
                    showToast("Emparejado");
                    BluetoothDevice dispositivo = (BluetoothDevice) mAdapter.getItem(posicionListBluethoot);

                    //se inicia el Activity de comunicacion con el bluethoot, para transferir los datos.
                    //Para eso se le envia como parametro la direccion(MAC) del bluethoot Arduino
                    String direccionBluethoot = dispositivo.getAddress();
//                    Intent i = new Intent(DeviceListActivity.this, activity_comunicacion.class);
//                    i.putExtra("Direccion_Bluethoot", direccionBluethoot);
//
//                    startActivity(i);

                }
                else if (state == BluetoothDevice.BOND_NONE && prevState == BluetoothDevice.BOND_BONDED) {
                    showToast("No emparejado");
                }

                mAdapter.notifyDataSetChanged();
            }
        }
    };
}

