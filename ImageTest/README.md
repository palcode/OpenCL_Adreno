##########################
##
##	ImageTest
##	Bartomiej Chabecki
##	b.chabecki@gmail.com
##
##########################

// This Readme is a stub

This program turns a full colour picture (http://www.derich.net/?p=141) into sepia.

It takes every pixel (RGBA) and calculates its new value with this formula:

A = A
R = R*0.393 + G*0.769 + B*0.189
G = R*0.349 + G*0.686 + B*0.168
B = R*0.272 + G*0.534 + B*0.131

Results are limited to 255.

Calculations are performed both normally and using OpenCL. App measures the execution time of both methods.

App assumes that your OpenCL.so is in /system/vendor/lib/