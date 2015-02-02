package com.chabecki.imagetest;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.SystemClock;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;


public class MainActivity extends ActionBarActivity {

    static {
        try {
            System.load("/system/vendor/lib/libOpenCL.so");
            Log.i("Images", "OpenCL lib Loaded");
            System.loadLibrary("Images");
            Log.i("Images", "My Lib Loaded!");
        }
        catch (UnsatisfiedLinkError e) {
            Log.i("Images", "Libraries ERROR!");
        }
    }

    private native void runOpenCL(Bitmap inputBitmap, Bitmap outputBitmap);

    Bitmap inputBitmap;
    Bitmap outputCPUBitmap;
    Bitmap outputOCLBitmap;
    ImageView outputCPU;
    ImageView outputOCL;
    TextView textCPU;
    TextView textOCL;
    ViewGroup vg;
    long startTime, difference;

    int imageWidth;
    int imageHeight;

    public void sepiaCPU(View v){
        int pixelColor;
        int newPixel;
        int R, G, B, A, nR, nG, nB;
        startTime = SystemClock.elapsedRealtime();
        for(int i=0; i < imageWidth; i++)
        {
            for(int j=0; j < imageHeight; j++)
            {
                pixelColor = inputBitmap.getPixel(i, j);
                R = Color.red(pixelColor);
                G = Color.green(pixelColor);
                B = Color.blue(pixelColor);
                A = Color.alpha(pixelColor);
                nR = (int) ((R * 0.393) + (G * 0.769) + (B * 0.189));
                nG = (int) ((R * 0.349) + (G * 0.686) + (B * 0.168));
                nB = (int) ((R * 0.272) + (G * 0.534) + (B * 0.131));
                if (nR > 255) nR = 255;
                if (nG > 255) nG = 255;
                if (nB > 255) nB = 255;
                newPixel = Color.argb(A,nR,nG,nB);
                outputCPUBitmap.setPixel(i, j, newPixel);
            }
        }
        difference = (SystemClock.elapsedRealtime() - startTime);
        outputCPU.setImageBitmap(outputCPUBitmap);
        textCPU.setText("CPU\nCzas wykonania obliczeń: " + Double.toString(difference/1000.0) + " sekund.");
    }

    public void sepiaOCL(View v){
        Log.i("Images", "Running OpenCL!");
        startTime = SystemClock.elapsedRealtime();
        runOpenCL(inputBitmap, outputOCLBitmap);
        difference = (SystemClock.elapsedRealtime() - startTime);
        Log.i("Images", "Done!");
        outputOCL.setImageBitmap(outputOCLBitmap);
        outputOCL.postInvalidate();
        textOCL.setText("OpenCL\nCzas wykonania obliczeń: " + Double.toString(difference/1000.0) + " sekund.");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //preview = (ImageView)findViewById(R.id.origIMG);
        outputCPU = (ImageView)findViewById(R.id.cpuIMG);
        outputOCL = (ImageView)findViewById(R.id.oclIMG);

        //textPreview = (TextView)findViewById(R.id.origTXT);
        textCPU = (TextView)findViewById(R.id.cpuTXT);
        textOCL = (TextView)findViewById(R.id.oclTXT);

        vg = (ViewGroup)findViewById(R.id.mainLayout);

        inputBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.flowers);
        imageWidth  = inputBitmap.getWidth();
        imageHeight = inputBitmap.getHeight();
        outputCPUBitmap = Bitmap.createBitmap(imageWidth, imageHeight, Bitmap.Config.ARGB_8888);
        outputOCLBitmap = Bitmap.createBitmap(imageWidth, imageHeight, Bitmap.Config.ARGB_8888);

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
