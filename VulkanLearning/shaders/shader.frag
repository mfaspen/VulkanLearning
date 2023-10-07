//#version 450
#extension GL_ARB_separate_shader_objects : enable
varying vec2 V_Texcoord;
uniform sampler2D U_Texture;
void main() {
	float colorB;
	float d1 = 1.020238308;
	float r1 = sqrt(1.0 +d1*d1);

	vec2 texcoordB = (V_Texcoord-0.5)*2.0;
	float bz1 = sqrt(d1*d1 + texcoordB.x*texcoordB.x + texcoordB.y*texcoordB.y)/ r1;

	float R = 2.8067297723162;
	float D  = sqrt(R*R -1.0);
	float bz2 = R/sqrt(D*D + texcoordB.x*texcoordB.x + texcoordB.y*texcoordB.y);

	texcoordB = vec2(texcoordB.x*bz1*bz2,texcoordB.y*bz1*bz2);
	texcoordB = texcoordB * 0.5 + 0.5;
	if(texcoordB.x >0.0 && texcoordB.x <1.0 &&texcoordB.y >0.0 && texcoordB.y <1.0){
		colorB = texture2D(U_Texture,texcoordB).b;
	}else{
		colorB =0.0;
	}





















	float colorG;
	float d2 = 1.2529824508;
	float r2 = sqrt(1.0 +d2*d2);
	vec2 texcoordG = (V_Texcoord-0.5)*2.0;
	float bz2 = sqrt(d2*d2 + texcoordG.x*texcoordG.x + texcoordG.y*texcoordG.y)/ r2;
	texcoordG = vec2(texcoordG.x*bz2,texcoordG.y*bz2);
	texcoordG = texcoordG * 0.5 + 0.5;
	if(texcoordG.x >0.0 && texcoordG.x <1.0 &&texcoordG.y >0.0 && texcoordG.y <1.0){
		colorG = texture2D(U_Texture,texcoordG).g;
	}else{
		colorG =0.0;
	}
	float colorR;
	float d3 = 1.2534145725;
	float r3 = sqrt(1.0 +d3*d3);
	vec2 texcoordR = (V_Texcoord-0.5)*2.0;
	float bz3 = sqrt(d3*d3 + texcoordR.x*texcoordR.x + texcoordR.y*texcoordR.y)/ r3;
	texcoordR = vec2(texcoordR.x*bz3,texcoordR.y*bz3);
	texcoordR = texcoordR * 0.5 + 0.5;
	if(texcoordR.x >0.0 && texcoordR.x <1.0 &&texcoordR.y >0.0 && texcoordR.y <1.0){
		colorR = texture2D(U_Texture,texcoordR).r;
	}else{
		colorR =0.0;
	}
	gl_FragColor = vec4 (colorR,colorG,colorB,1.0);


//	float d1 = 1.8;
//	float r1 = sqrt(1.0 +d1*d1);
//	vec2 texcoord1 = (V_Texcoord-0.5)*2.0;
//	texcoord1.x =  texcoord1.x*1.7777777777;;
//	float bz1 = sqrt(d1*d1 + texcoord1.x*texcoord1.x + texcoord1.y*texcoord1.y)/ r1;
//	texcoord1 = vec2(texcoord1.x*bz1,texcoord1.y*bz);
//	texcoord1.x =  texcoord1.x*0.5625;
//	texcoord1 = texcoord1 * 0.5 + 0.5;
//	vec4 color2 = texture2D(U_Texture,texcoord1);
//	float d2 = 1.6;
//	float r2 = sqrt(1.0 +d2*d2);
//	vec2 texcoord2 = (V_Texcoord-0.5)*2.0;
//	texcoord2.x =  texcoord2.x*1.7777777777;;
//	float bz2 = sqrt(d2*d2 + texcoord2.x*texcoord2.x + texcoord2.y*texcoord2.y)/ r2;
//	texcoord2 = vec2(texcoord2.x*bz2,texcoord2.y*bz2);
//	texcoord2.x =  texcoord2.x*0.5625;
//	texcoord2 = texcoord2 * 0.5 + 0.5;
//	vec4 color3 = texture2D(U_Texture,texcoord2);
//	gl_FragColor = vec4 (color1.r,color2.g,color3.b,1.0);

}