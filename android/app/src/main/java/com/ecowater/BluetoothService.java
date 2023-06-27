package com.ecowater;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

public class BluetoothService {
    public static final int STATE_NONE = 0;       // we're doing nothing
    public static final int STATE_CONNECTING = 2; // now initiating an outgoing connection
    public static final int STATE_CONNECTED = 3;

    //private static final UUID BTMODULEUUID = UUID.fromString("7fd7c9c5-771d-43f3-9a15-8af772bfdd68");
    private static final UUID BT_UUID_SECURE = UUID.fromString("fa87c0d0-afac-11de-8a39-0800200c9a66");
    private static final UUID BT_UUID_INSECURE = UUID.fromString("8ce255c0-200a-11e0-ac64-0800200c9a66");
    private final BluetoothAdapter aBluetoothAdapter;
    private final Handler aHandler;
    private ConnectThread aConnectThread;
    private ConnectedThread aConnectedThread;
    private int aState;
    private int aNewState;

    public BluetoothService(Context context, Handler handler) {
        aBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        aState = STATE_NONE;
        aNewState = aState;
        aHandler = handler;
    }

    private synchronized void updateStatusBarTitle() {
        aState = getState();
        aNewState = aState;

        aHandler.obtainMessage(Constants.MESSAGE_STATE_CHANGE, aNewState, -1).sendToTarget();
    }

    public synchronized int getState() {
        return aState;
    }

    public synchronized void start() {

        // Cancelamos cualquier intento conexion en proceso
        if (aConnectThread != null) {
            aConnectThread.cancel();
            aConnectThread = null;
        }

        // Cancelamos cualquier conexion en proceso
        if (aConnectedThread != null) {
            aConnectedThread.cancel();
            aConnectedThread = null;
        }

        updateStatusBarTitle();
    }

    public synchronized void connect(BluetoothDevice device, boolean secure) {

        // Cancelamos cualquier intento conexion en proceso
        if (aState == STATE_CONNECTING) {
            if (aConnectThread != null) {
                aConnectThread.cancel();
                aConnectThread = null;
            }
        }

        // Cancelamos cualquier conexion en proceso
        if (aConnectedThread != null) {
            aConnectedThread.cancel();
            aConnectedThread = null;
        }

        // Intentamos conectarnos con el dispositivo
        aConnectThread = new ConnectThread(device, secure);
        aConnectThread.start();

        updateStatusBarTitle();
    }

    @SuppressLint("MissingPermission")
    public synchronized void connected(BluetoothSocket socket, BluetoothDevice device, final String socketType) {

        // Cancelamos el thread de conexion
        if (aConnectThread != null) {
            aConnectThread.cancel();
            aConnectThread = null;
        }

        // Cancelamos cualquier thread intentando conectarse
        if (aConnectedThread != null) {
            aConnectedThread.cancel();
            aConnectedThread = null;
        }

        // Empezamos el thread que maneja la conexion ya hecha con el dispositivo (datos)
        aConnectedThread = new ConnectedThread(socket, socketType);
        aConnectedThread.start();

        // Enviamos el nombre del dispositivo conectado a ArduinoActivity
        Message msg = aHandler.obtainMessage(Constants.MESSAGE_DEVICE_NAME);
        Bundle bundle = new Bundle();
        bundle.putString(Constants.DEVICE_NAME, device.getName());
        msg.setData(bundle);
        aHandler.sendMessage(msg);

        updateStatusBarTitle();
    }

    // Cancelacion de threads
    public synchronized void stop() {
        if (aConnectThread != null) {
            aConnectThread.cancel();
            aConnectThread = null;
        }

        if (aConnectedThread != null) {
            aConnectedThread.cancel();
            aConnectedThread = null;
        }

        aState = STATE_NONE;
        updateStatusBarTitle();
    }


    // Escritura en thread de datos
    public void write(byte[] out) {
        ConnectedThread r;

        synchronized (this) {
            if (aState != STATE_CONNECTED) return;
            r = aConnectedThread;
        }

        r.write(out);
    }

    // Conexion fallida
    private void connectionFailed() {
        Message msg = aHandler.obtainMessage(Constants.MESSAGE_TOAST);
        Bundle bundle = new Bundle();
        bundle.putString(Constants.TOAST, "No fue posible conectarse al dispositivo");
        msg.setData(bundle);
        aHandler.sendMessage(msg);

        aState = STATE_NONE;

        updateStatusBarTitle();

        // Restarteamos el servicio para volver a escuchar
        BluetoothService.this.start();
    }

    // Conexion perdida
    private void connectionLost() {
        Message msg = aHandler.obtainMessage(Constants.MESSAGE_TOAST);
        Bundle bundle = new Bundle();
        bundle.putString(Constants.TOAST, "Se perdio la conexion con el dispositivo");
        msg.setData(bundle);
        aHandler.sendMessage(msg);

        aState = STATE_NONE;

        updateStatusBarTitle();

        // Restarteamos el servicio para volver a escuchar
        BluetoothService.this.start();
    }


    // Thread que intenta realizar la conexion con el dispositivo bluetooth
    private class ConnectThread extends Thread {
        private final BluetoothSocket aSocket;
        private final BluetoothDevice aDevice;
        private String aSocketType;

        @SuppressLint("MissingPermission")
        public ConnectThread(BluetoothDevice device, boolean secure) {
            aDevice = device;
            BluetoothSocket tmp = null;
            aSocketType = secure ? "Secure" : "Insecure";

            try {
                if (secure) {
                    tmp = device.createRfcommSocketToServiceRecord(BT_UUID_SECURE);
                } else {
                    tmp = device.createInsecureRfcommSocketToServiceRecord(BT_UUID_INSECURE);
                }
            } catch (IOException e) {
            }
            aSocket = tmp;
            aState = STATE_CONNECTING;
        }

        @SuppressLint("MissingPermission")
        public void run() {
            aBluetoothAdapter.cancelDiscovery();

            try {
                aSocket.connect();
            } catch (IOException e) {
                try {
                    aSocket.close();
                } catch (IOException e2) {
                    e2.printStackTrace();
                }
                e.printStackTrace();
                connectionFailed();
                return;
            }

            // Reseteamos el thread
            synchronized (BluetoothService.this) {
                aConnectThread = null;
            }

            // Comenzamos el thread de datos
            connected(aSocket, aDevice, aSocketType);
        }

        public void cancel() {
            try {
                aSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    // Thread que envia y recibe datos de los dispositivos conectados
    private class ConnectedThread extends Thread {
        private final BluetoothSocket aSocket;
        private final InputStream anInputStream;
        private final OutputStream anOutputStream;

        public ConnectedThread(BluetoothSocket socket, String socketType) {
            aSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) {
                e.printStackTrace();
            }

            anInputStream = tmpIn;
            anOutputStream = tmpOut;
            aState = STATE_CONNECTED;
        }

        public void run() {
            byte[] buffer = new byte[1024];
            int bytes;

            while (aState == STATE_CONNECTED) {

                // Leemos y enviamos al handler
                try {
                    bytes = anInputStream.read(buffer);
                    aHandler.obtainMessage(Constants.MESSAGE_READ, bytes, -1, buffer).sendToTarget();
                } catch (IOException e) {
                    e.printStackTrace();
                    connectionLost();
                    break;
                }
            }
        }

        public void write(byte[] buffer) {

            // Escribimos y enviamos al handler
            try {
                anOutputStream.write(buffer);
                aHandler.obtainMessage(Constants.MESSAGE_WRITE, -1, -1, buffer).sendToTarget();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        public void cancel() {
            try {
                aSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}
