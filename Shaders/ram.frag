varying vec4 vecta;
varying vec4 thisColor;

uniform float poop;

void main()
	{	
	      float fade = 1.2 - vecta.z * .8; 
	      float flashiness = poop / 10.0;
	      float flashColor = cos(flashiness);
	      float otherColor = sin(flashiness);
	      //float nextColor = tan(flashiness);
            float fader = fade;
            flashColor = abs(flashColor) + .5;
            otherColor = abs(otherColor) - .5;
            //nextColor = abs(nextColor);
	      vec4 color = vec4(0.0, otherColor, flashColor, fader);

		gl_FragColor = color;	
	}
