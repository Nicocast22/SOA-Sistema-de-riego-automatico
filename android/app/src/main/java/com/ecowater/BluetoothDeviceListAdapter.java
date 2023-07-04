package com.ecowater;


import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.TextView;

import java.util.List;

public class BluetoothDeviceListAdapter extends BaseAdapter {
    private LayoutInflater anInflater;
    private List<BluetoothDevice> devicesList;
    private ButtonOnClickListener aListener;

    public BluetoothDeviceListAdapter(Context context) {
        anInflater = LayoutInflater.from(context);
    }

    public void setData(List<BluetoothDevice> data) {
        devicesList = data;
    }

    public void setListener(ButtonOnClickListener listener) {
        aListener = listener;
    }

    public int getCount() {
        return devicesList.size();
    }

    public Object getItem(int position) {
        return devicesList.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    @SuppressLint("MissingPermission")
    public View getView(final int position, View convertView, ViewGroup parent) {
        ViewHolder holder;

        if (convertView == null) {
            convertView = anInflater.inflate(R.layout.bluetooth_device, null);

            holder = new ViewHolder();

            holder.deviceName = (TextView) convertView.findViewById(R.id.device_name);
            holder.deviceMacAddress = (TextView) convertView.findViewById(R.id.device_mac_address);
            holder.pairBtn = (Button) convertView.findViewById(R.id.pairBtn);

            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        BluetoothDevice device = devicesList.get(position);

        holder.deviceName.setText(device.getName());
        holder.deviceMacAddress.setText(device.getAddress());
        holder.pairBtn.setText((device.getBondState() == BluetoothDevice.BOND_BONDED) ? "Desemparejar" : "Emparejar");
        holder.pairBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (aListener != null) {
                    aListener.onButtonClick(position);
                }
            }
        });

        return convertView;
    }

    public interface ButtonOnClickListener {
        public abstract void onButtonClick(int position);
    }

    static class ViewHolder {
        TextView deviceName;
        TextView deviceMacAddress;
        TextView pairBtn;
    }
}
