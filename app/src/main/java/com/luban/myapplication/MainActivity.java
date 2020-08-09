package com.luban.myapplication;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    SurfaceView surfaceView ;
    LubanPlayer player;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        player = new LubanPlayer();
        surfaceView =findViewById(R.id.surfaceview);
        player.setSurfaceView(surfaceView);
        player.setDateSource("rtmp://58.200.131.2:1935/livetv/hunantv");
        player.setPrepareListener(new LubanPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                Log.i("MainActivity","运行成功了");
                runOnUiThread(new Runnable() {
                    @SuppressLint("WrongConstant")
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "可以开始播放了", 0).show();
                    }

                });
                //开始播放
                player.startPlayer();
            }
        });
        findViewById(R.id.start).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.i("MainActivity","开始运行");
                player.prepare();
            }
        });
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stopPlayer();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }
}
