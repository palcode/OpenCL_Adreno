package com.chabecki.smallbenchmark;

import android.os.SystemClock;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.util.Random;


public class MainActivity extends ActionBarActivity {

    // Otwieranie potrzebnych bibliotek
    static boolean sfoundLibrary = true;
    static {
        try {
            System.load("/system/vendor/lib/libOpenCL.so");
            Log.i("Debug", "OpenCL lib Loaded");
            System.loadLibrary("Vectors");
            Log.i("Debug","My Lib Loaded!");
        }
        catch (UnsatisfiedLinkError e) {
            sfoundLibrary = false;
        }
    }

    private native void initOpenCL(float[] avec, float[] bvec, int rozmiar);
    private native void vectorOpenCL(float[] open_cvec);
    private native void cleanOpenCL();
   // private native void shutdownOpenCL ();

    Button start;
    TextView status;
    EditText rozmiarWektora;

    int rozmiar = 102400;

    Random gen = new Random();
    float[] avec = new float[rozmiar];
    float[] bvec = new float[rozmiar];
    float[] cvec = new float[rozmiar];
    float[] open_cvec = new float[rozmiar];


    public void buttonKlik(View v){
        rozmiar = Integer.parseInt(rozmiarWektora.getText().toString());
        status.setText("");
        status.append("\nRozpoczynam losowanie dwóch wektorów float x"+rozmiar+"...");
        for(int i = 0; i<rozmiar; i++){
            avec[i] = gen.nextFloat();
            bvec[i] = gen.nextFloat();
        }
        status.append("\nWylosowano. Rozpoczynam obliczenia tradycyjną metodą...");
        long startTime = SystemClock.elapsedRealtimeNanos();
        for(int i = 0; i<rozmiar; i++){
            cvec[i] = avec[i] + bvec[i];
            cvec[i] *= avec[i];
            cvec[i] *= bvec[i];
            cvec[i] *= cvec[i];
            cvec[i] *= 10.0;
        }
        long difference = SystemClock.elapsedRealtimeNanos() - startTime;
        String czas = Long.toString(difference);
        status.append("\nCzas wykonania obliczeń: "+czas+" nanosekund.");
        // Ładuję kernela i odpalam
        status.append("\nŁaduję  wektory do pamięci, kompiluję kernela itepe...");
        initOpenCL(avec, bvec, rozmiar);
        status.append("\nCzas na właściwe wykonanie programu w urządzeniu!");
        startTime = SystemClock.elapsedRealtimeNanos();
        vectorOpenCL(open_cvec);
        long difference2 = SystemClock.elapsedRealtimeNanos() - startTime;
        String czas2 = Long.toString(difference2);
        String roznica =  Long.toString(difference-difference2);
        status.append("\nCzas wykonania (tylko) obliczeń: "+czas2+" nanosekund.");
        status.append("\nRóżnica: "+roznica+" nanosekund.");
        cleanOpenCL();


        /*for(int i = 0; i<rozmiar; i++){
            status.append("\n"+cvec[i]+"\t"+open_cvec[i]);
        }*/

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        start =(Button) findViewById(R.id.startButton);
        status = (TextView)  findViewById(R.id.statusBox);
        rozmiarWektora = (EditText) findViewById(R.id.rozmiaryWektora);
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
