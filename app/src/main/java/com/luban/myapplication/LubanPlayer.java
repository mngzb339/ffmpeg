package com.luban.myapplication;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class LubanPlayer implements SurfaceHolder.Callback{

    static {
        System.loadLibrary("native-lib");
    }
    String dateSource;
    SurfaceView surfaceView;
    SurfaceHolder holder;
    OnPrepareListener listener;
    public void setDateSource(String source) {
        dateSource = source;
    }
    /**
     * 开始播放
     */
    public void startPlayer() {
         native_start();
    }

    /**
     * 停止播放
     */
    public void stopPlayer() {

    }

    /**
     * 释放
     */
    public void release() {
        holder.removeCallback(this);
    }

    /**
     * 设置setSurfaceView
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        this.surfaceView = surfaceView;
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    public void onError(int errorCode){

    }
    /**
     * 准备播放
     */
    public void prepare() {
        native_prepare(dateSource);
    }

    public void onPrepare(){
        if(null!=listener){
            Log.i("MainActivity","结束");

            listener.onPrepare();
        }
    }
    public void setPrepareListener(OnPrepareListener listener){
        this.listener = listener;
    }

    /**
     * 准备好 回调
     */
    public interface OnPrepareListener{
        void onPrepare();
    }
    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
       // native_SetSurface(holder.getSurface());
    }

    /**
     * 横竖屏幕 或者按了home按键
     * @param surfaceHolder
     * @param i
     * @param i1
     * @param i2
     */
    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        native_SetSurface(holder.getSurface());

    }

    /**
     * 按了home 或者推出应用
     * @param surfaceHolder
     */
    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    public native void native_prepare(String dateSource);

    public native void native_start();

    public native void native_SetSurface(Surface surface);

}
