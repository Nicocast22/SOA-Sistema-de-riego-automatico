package com.ecowater;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.List;

public class BluetoothActivity extends AppCompatActivity {

    private static final int PERMISSIONS_CODE = 4;
    private final static int REQUEST_ENABLE_BT = 1000;

    String[] permissions = new String[]{Manifest.permission.BLUETOOTH, Manifest.permission.ACCESS_COARSE_LOCATION, Manifest.permission.ACCESS_BACKGROUND_LOCATION, Manifest.permission.ACCESS_FINE_LOCATION};

    private BluetoothAdapter aBluetoothAdapter;

    private ArrayList<BluetoothDevice> aDeviceList = new ArrayList<>();

    private Button toggleBtBtn;
    private Button searchDevicesBtBtn;
    private ProgressBar progressBar;
    private TextView onText;
    private TextView offText;

    // Bluetooth
    private final BroadcastReceiver aReceiver = new BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            switch (action) {
                case BluetoothAdapter.ACTION_STATE_CHANGED:
                    final int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                    showBluetoothStatusAndUpdateLayout(state);
                    break;

                case BluetoothAdapter.ACTION_DISCOVERY_STARTED:
                    progressBar.setVisibility(View.VISIBLE);
                    aDeviceList = new ArrayList<BluetoothDevice>();
                    showToast(getString(R.string.bt_device_search_init));
                    getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                    break;

                case BluetoothAdapter.ACTION_DISCOVERY_FINISHED:
                    progressBar.setVisibility(View.INVISIBLE);
                    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
                    showToast(getString(R.string.bt_device_search_end));

                    Intent newIntent = new Intent(BluetoothActivity.this, BluetoothDevicesActivity.class);
                    newIntent.putParcelableArrayListExtra("device.list", aDeviceList);
                    startActivity(newIntent);
                    break;

                case BluetoothDevice.ACTION_FOUND:
                    BluetoothDevice device = (BluetoothDevice) intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    aDeviceList.add(device);
                    showToast(getString(R.string.device_found) + device.getName());
                    break;
            }
        }
    };

    // Listeners
    private void enableAndInitializeButtons() {
        toggleBtBtn.setOnClickListener(new View.OnClickListener() {
            @SuppressLint("MissingPermission")
            public void onClick(View v) {
                if (aBluetoothAdapter.isEnabled()) {
                    aBluetoothAdapter.disable();
                    showBluetoothStatusAndUpdateLayout(BluetoothAdapter.STATE_OFF);
                } else {
                    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
                }
            }
        });

        searchDevicesBtBtn.setOnClickListener(new View.OnClickListener() {
            @SuppressLint("MissingPermission")
            public void onClick(View v) {
                aBluetoothAdapter.startDiscovery();
            }
        });
    }

    // Lifecycle
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bluetooth);
        setTitle(getString(R.string.bluetooth));

        toggleBtBtn = (Button) findViewById(R.id.toggleBtBtn);
        searchDevicesBtBtn = (Button) findViewById(R.id.searchDevicesBtn);
        progressBar = (ProgressBar) findViewById(R.id.progressBar);
        onText = (TextView) findViewById(R.id.btOnText);
        offText = (TextView) findViewById(R.id.btOffText);

        progressBar.setVisibility(View.INVISIBLE);

        aBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        checkForPermissions();
        refreshLayout();
        createIntentFilterAndRegisterReceiver();
    }

    @SuppressLint("MissingPermission")
    public void onPause() {
        if (aBluetoothAdapter != null) {
            if (aBluetoothAdapter.isDiscovering()) {
                aBluetoothAdapter.cancelDiscovery();
            }
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        unregisterReceiver(aReceiver);
        super.onDestroy();
    }

    // Bluetooth utils
    private boolean isBluetoothAvailable() {
        return aBluetoothAdapter != null;
    }

    private void createIntentFilterAndRegisterReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);

        registerReceiver(aReceiver, filter);
    }

    // Permissions
    private void checkForPermissions() {
        if (!isBluetoothAvailable()) {
            showToast(getString(R.string.bt_not_available_on_device));
            finish();
        }

        int result;
        List<String> listPermissionsNeeded = new ArrayList<>();

        for (String p : permissions) {
            result = ContextCompat.checkSelfPermission(this, p);
            if (result != PackageManager.PERMISSION_GRANTED) {
                listPermissionsNeeded.add(p);
            }
        }
        if (!listPermissionsNeeded.isEmpty()) {
            ActivityCompat.requestPermissions(this, listPermissionsNeeded.toArray(new String[listPermissionsNeeded.size()]), PERMISSIONS_CODE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case PERMISSIONS_CODE:
                if (grantResults.length > 0 &&
                        grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    showToast("Permisos OK");
                } else {
                    showToast("Permisos denegados");
                    finish();
                }
                return;
        }
    }

    // Layout utils
    protected void refreshLayout() {
        showBluetoothStatusAndUpdateLayout(aBluetoothAdapter.getState());
        enableAndInitializeButtons();
    }

    private void showToast(String message) {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }

    private void updateButtonsAndShowText(boolean toggleBtnStatus, boolean searchDevicesBtBtnStatus, String warning) {
        toggleBtBtn.setEnabled(toggleBtnStatus);
        searchDevicesBtBtn.setEnabled(searchDevicesBtBtnStatus);

        showToast(warning);
    }

    private void showBluetoothStatusAndUpdateLayout(int bluetoothStatus) {
        switch (bluetoothStatus) {
            case BluetoothAdapter.STATE_TURNING_OFF:
                showToast(getString(R.string.turning_bt_off));
                break;

            case BluetoothAdapter.STATE_OFF:
                updateButtonsAndShowText(true, false, getString(R.string.bt_disabled));
                toggleBtBtn.setText(getString(R.string.enable_bt));
                onText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.disabledSuccess));
                offText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.semanticError));
                break;

            case BluetoothAdapter.STATE_TURNING_ON:
                showToast(getString(R.string.turning_bt_on));
                break;

            case BluetoothAdapter.STATE_ON:
                updateButtonsAndShowText(true, true, getString(R.string.bt_enabled));
                toggleBtBtn.setText(getString(R.string.disable_bt));
                onText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.semanticSuccess));
                offText.setBackgroundColor(ContextCompat.getColor(getApplicationContext(), R.color.disabledError));
                break;
        }
    }

}