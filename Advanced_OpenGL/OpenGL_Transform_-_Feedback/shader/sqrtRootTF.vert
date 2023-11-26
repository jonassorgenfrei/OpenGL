#version 330 core

in float inValue; // input single arbitrary float 
out float outValue;

void main()
{
    // NOTE: no gl_Position set

    // calculate square root of input values
    outValue = sqrt(inValue);
}