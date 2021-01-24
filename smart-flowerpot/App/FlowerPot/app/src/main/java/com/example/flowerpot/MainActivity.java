package com.example.flowerpot;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;

import com.white.progressview.CircleProgressView;
import com.white.progressview.HorizontalProgressView;

public class MainActivity extends AppCompatActivity {

    // Timer
    private TimerTask refresh;
    private final Handler handler = new Handler();
    private final int REFRESH_FREQUENCY = 60000; // 60*1000ms

    public static final int INTERNET_PERMISSION_CODE = 1;

    // 서버에서 가져온 값 처리하는 변수들
    String jsonData;

    // 주소 변경 필요
    String urlGetData="http://도메인 주소/getData.php";

    private static final String TAG_RESULTS = "result";
    private static final String TAG_TEMP = "temp";
    private static final String TAG_HUMID = "humid";
    private static final String TAG_SOILHUMID = "soil_humid";
    private static final String TAG_UV = "uv";
    private static final String TAG_MEASURETIME = "measureTime";

    private TextView txtTemp;
    private TextView txtHumid;
    private TextView txtUV;
    private TextView txtSoil;
    private TextView txtMeasureTime;
    private ProgressBar progressTemp;
    private ProgressBar progressHumid;
    private CircleProgressView progressSoilHumid;

    JSONArray measureLog = null;

    private int temp;
    private int humid;
    private int soil_humid;
    private float uv;
    private String measureTime;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if(Build.VERSION.SDK_INT >= 23 && !checkPermissions()) {
            requestPermissions();
        }

        txtTemp = findViewById(R.id.temp);
        txtHumid = findViewById(R.id.humid);
        txtUV = findViewById(R.id.uv);
        txtSoil = findViewById(R.id.soil);
        txtMeasureTime = findViewById(R.id.measureTime);

        progressTemp = findViewById(R.id.progressTemp);
        progressHumid = findViewById(R.id.progressHumid);
        progressSoilHumid = findViewById(R.id.progressSoil);

        // 앱을 켜면 최초 1회 서버에서 자료를 가져옴
        getData(urlGetData);

        // 정해진 시간(1분)마다 자동 새로고침하는 타이머 시작
        startTimer();
    }

    // 데이터 읽어오기 및 처리
    protected void showData() {
        try {
            JSONObject jsonObj = new JSONObject(jsonData);
            measureLog = jsonObj.getJSONArray(TAG_RESULTS);

            for (int i = 0; i < measureLog.length(); i++) {

                JSONObject c = measureLog.getJSONObject(i);
                temp = Integer.parseInt(c.getString(TAG_TEMP));
                humid = Integer.parseInt(c.getString(TAG_HUMID));
                soil_humid = Integer.parseInt(c.getString(TAG_SOILHUMID));
                uv = Float.parseFloat(c.getString(TAG_UV));
                measureTime = c.getString(TAG_MEASURETIME);

                txtTemp.setText("온도 : " + c.getString(TAG_TEMP) + "C");
                txtHumid.setText("습도 : " + c.getString(TAG_HUMID) + "%");
                txtSoil.setText("토양 습도 : " + c.getString(TAG_SOILHUMID) + "%");
                txtMeasureTime.setText("측정 시각 : " + measureTime);
                txtUV.setText(c.getString(TAG_UV) + "mW");

                progressTemp.setProgress(temp);
                progressHumid.setProgress(humid);

                if(soil_humid < 30) {
                    progressSoilHumid.setReachBarColor(Color.parseColor("FFFF0000"));
                }
                else if(soil_humid < 50) {
                    progressSoilHumid.setReachBarColor(Color.parseColor("FFFFFF00"));
                }
                else {
                    progressSoilHumid.setReachBarColor(Color.parseColor("FF00FF00"));
                }

                progressSoilHumid.setProgress(soil_humid);
            }
        } catch (Exception ex) {

        }
    }


    // 데이터 수신 부분
    public void getData(String url) {
        class GetDataJSON extends AsyncTask<String, Void, String> {
            @Override
            protected String doInBackground(String... params) {
                String uri = params[0];

                BufferedReader bufferedReader = null;
                try {
                    URL url = new URL(uri);
                    HttpURLConnection conn=(HttpURLConnection) url.openConnection();
                    StringBuilder sb = new StringBuilder();
                    conn.setConnectTimeout(10000);
                    conn.setReadTimeout(10000);
                    conn.setRequestMethod("POST");
                    conn.setDoInput(true);
                    conn.connect();

                    bufferedReader = new BufferedReader(new InputStreamReader(conn.getInputStream()));

                    String json;
                    while((json=bufferedReader.readLine())!=null) {
                        sb.append(json+'\n');
                    }

                    conn.disconnect();
                    return sb.toString().trim();

                } catch (Exception ex) {
                    Toast.makeText(getApplication(),"데이터 전송 준비 과정 중 오류 발생",Toast.LENGTH_SHORT).show();
                    return null;
                }
            }

            @Override
            protected void onPostExecute(String result) {
                jsonData=result;
                showData();
            }
        }
        GetDataJSON g = new GetDataJSON();
        g.execute(url);
    }

    // 퍼미션(INTERNET) 처리 부분
    private boolean checkPermissions() {
        if(ContextCompat.checkSelfPermission(MainActivity.this,Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
            return false;
        }
        return true;
    }

    private void requestPermissions() {
        if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.INTERNET)) {
            new AlertDialog.Builder(this)
                    .setTitle("권한 요청")
                    .setMessage("서버 연동을 위한 권한 요청입니다.")
                    .setPositiveButton("허용", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            ActivityCompat.requestPermissions(MainActivity.this, new String[]{Manifest.permission.INTERNET}, INTERNET_PERMISSION_CODE);
                        }
                    })
                    .setNegativeButton("거부", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    })
                    .create().show();
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.INTERNET}, INTERNET_PERMISSION_CODE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        if(requestCode == INTERNET_PERMISSION_CODE) {
            if(grantResults.length>0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "권한 요청이 승인되었습니다.", Toast.LENGTH_SHORT).show();
            }
            else {
                Toast.makeText(this, "권한 요청이 거부되었습니다.", Toast.LENGTH_SHORT).show();
            }
        }
    }

    // 자동 새로 고침, 새로고침 주기 1분
    private void startTimer() {
        refresh=new TimerTask() {
            @Override
            public void run() {
                Log.i("Timer","Timer Start");
                getData(urlGetData);
            }
        };
        Timer timer = new Timer();
        timer.schedule(refresh,REFRESH_FREQUENCY,REFRESH_FREQUENCY);
    }
}
