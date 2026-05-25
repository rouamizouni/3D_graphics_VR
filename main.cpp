#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <cmath>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// PUZZLE SHADER
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* pV = R"(
#version 410 core
layout(location=0) in vec3 pos;
layout(location=1) in vec2 uv;
layout(location=2) in vec3 normal;
uniform mat4 MVP; uniform mat4 model;
out vec2 TC; out vec3 N;
void main(){ TC=uv; N=normalize(mat3(transpose(inverse(model)))*normal); gl_Position=MVP*vec4(pos,1); }
)";
const char* pF = R"(
#version 410 core
in vec2 TC; in vec3 N;
uniform sampler2D tex; uniform int hasTex;
out vec4 F;
void main(){
    vec4 c=hasTex==1?texture(tex,TC):mix(vec4(.9,.5,.1,1),vec4(.1,.3,.8,1),mod(floor(TC.x*6)+floor(TC.y*6),2.));
    vec3 n=normalize(N);
    float l=.4+max(dot(n,normalize(vec3(1,2,3))),0.)*.5+max(dot(n,normalize(vec3(-1,1,-2))),0.)*.2;
    F=vec4(c.rgb*l,1);
}
)";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// ORB SHADER
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* oV = R"(
#version 410 core
layout(location=0) in vec3 pos;
uniform mat4 MVP; out vec3 lp;
void main(){ lp=pos; gl_Position=MVP*vec4(pos,1); }
)";
const char* oF = R"(
#version 410 core
in vec3 lp; uniform vec3 color; uniform float time,ps;
out vec4 F;
void main(){
    float d=length(lp); if(d>1.) discard;
    float pulse=1.+sin(time*ps)*.35;
    float core=1.-smoothstep(0.,.25,d);
    float glow=pow(1.-d,2.5)*1.8;
    float rim=pow(d,3.)*smoothstep(.6,1.,d)*2.5;
    float ring1=(1.-smoothstep(0.,.04,abs(d-.55)))*.8;
    float ring2=(1.-smoothstep(0.,.03,abs(d-.75)))*.4;
    float hl=pow(max(dot(normalize(lp),normalize(vec3(-.5,.7,.5))),0.),12.)*1.5;
    vec3 c=color*(core*3.+glow+rim+ring1+ring2)*pulse+vec3(1)*hl+color*.3*ring1*pulse;
    F=vec4(c,clamp((glow+core*.8+ring1*.5)*pulse,0.,1.));
}
)";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// RAY SHADER
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* rV = R"(
#version 410 core
layout(location=0) in vec3 pos; layout(location=1) in vec2 uv;
uniform mat4 VP; out vec2 bUV;
void main(){ bUV=uv; gl_Position=VP*vec4(pos,1); }
)";
const char* rF = R"(
#version 410 core
in vec2 bUV; uniform vec3 color; uniform float alpha,time;
out vec4 F;
void main(){
    float w=pow(1.-abs(bUV.y),2.5);
    float core=1.-smoothstep(0.,.25,abs(bUV.y));
    float wave=.7+.3*sin(bUV.x*8.-time*3.);
    vec3 c=color*(w*.6+core*.8)*wave*2.;
    F=vec4(c,clamp((w*.6+core*.8)*wave*alpha*1.5,0.,1.));
}
)";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// STAR SHADER
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* sV = R"(
#version 410 core
layout(location=0) in vec3 pos; layout(location=1) in float br;
uniform mat4 VP; out float Br;
void main(){ Br=br; gl_Position=VP*vec4(pos,1); gl_PointSize=br*6.+2.; }
)";
const char* sF = R"(
#version 410 core
in float Br; uniform float time; out vec4 F;
void main(){
    vec2 c=gl_PointCoord-.5; if(length(c)>.5) discard;
    float tw=.5+.5*sin(time*2.5+Br*13.7);
    float g=1.-smoothstep(0.,.5,length(c));
    F=vec4(1,1,1,g*tw*Br);
}
)";

// Shooting star shader
const char* shootV = R"(
#version 410 core
layout(location=0) in vec3 pos;
layout(location=1) in float phase;
uniform mat4 VP; uniform float time;
out float alpha; out vec3 col; out float trail;
void main(){
    float t=fract(time*0.25+phase); // slower
    // Each star shoots across the sky
    float speed=t;
    // Direction: diagonal across sky
    vec3 dir=normalize(vec3(-2.5,-0.8,-1.0));
    vec3 p=pos+dir*speed*120.;
    alpha=max(0.,(1.-t)*smoothstep(0.,.05,t));
    trail=speed;
    // White-blue shooting star color
    col=mix(vec3(1.,1.,1.),vec3(.7,.85,1.),t);
    gl_PointSize=max(1.,(1.-t)*6.+1.);
    gl_Position=VP*vec4(p,1);
}
)";const char* shootF = R"(
#version 410 core
in float alpha; in vec3 col; in float trail;
out vec4 F;
void main(){
    vec2 c=gl_PointCoord-.5;
    // Elongated streak shape — narrow in X, long in Y
    float d=length(c*vec2(4.,1.));
    if(d>0.5) discard;
    float g=1.-smoothstep(0.,.5,d);
    // Bright core, color tail
    vec3 finalCol=mix(vec3(1.),col,trail);
    F=vec4(finalCol,g*alpha*1.5);
}
)";
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// SKY SHADER (row 1)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* skyV = R"(
#version 410 core
layout(location=0) in vec3 pos; uniform mat4 MVP; out vec3 wP;
void main(){ wP=pos; gl_Position=MVP*vec4(pos,1); }
)";
const char* skyF = R"(
#version 410 core
in vec3 wP;
uniform float reveal, reveal2, reveal3, time;
out vec4 F;
void main(){
    vec3 n = normalize(wP);
    float h = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);

    // STAGE 1: Dark blue sky — row 1
    vec3 blue = mix(vec3(0.06,0.18,0.55), vec3(0.01,0.05,0.28), h);
    vec3 col = blue * reveal;

    // STAGE 2: Orange horizon + sun — row 2
    float horiz = pow(1.0 - h, 4.0);
    vec3 orangeCol = mix(vec3(0.9,0.3,0.05), vec3(1.0,0.5,0.05), pow(1.0-h,6.0));
    col += orangeCol * horiz * 5.0 * reveal2;
    float band = pow(max(1.0 - abs(h-0.06)/0.10, 0.0), 2.0) * 2.0;
    col += vec3(1.0, 0.45, 0.02) * band * reveal2;
    // Sun disc
    vec3 sun = normalize(vec3(0.4, -0.1, -1.0));
    float sd = dot(n, sun);
    // Discard below horizon AND when looking underwater
    if(n.y < -0.01) discard;
    if(h < 0.02) discard;
    float aboveHorizon=smoothstep(0.0,0.08,h);
    col += vec3(1.0,0.95,0.5) * smoothstep(0.990,1.0,sd) * 5.0 * reveal2 * aboveHorizon;
    col += vec3(1.0,0.6,0.1) * pow(max(sd,0.0),40.0) * 1.2 * reveal2 * aboveHorizon;
    col += vec3(1.0,0.4,0.05)* pow(max(sd,0.0),15.0) * 0.3 * reveal2 * aboveHorizon;

    // STAGE 3: Yellow-gold transition band — row 3
    float yellowH = pow(max(1.0 - abs(h-0.20)/0.18, 0.0), 2.0);
    col += vec3(1.0, 0.6, 0.1) * yellowH * 1.2 * reveal3 * (1.0-h);

    F = vec4(clamp(col, 0.0, 1.0), 1.0);
}
)";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// WATER SHADER (row 2 + 3 + 4)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* wV = R"(
#version 410 core
layout(location=0) in vec3 pos;
layout(location=1) in vec2 uv;
uniform mat4 MVP; uniform float time, waveRev;
out vec2 UV; out float wH; out float dep;
out vec3 wNorm; out vec3 fragPos; out vec4 vColor;

vec3 gerstner(vec4 wave, vec3 p){
    float steepness=wave.z;
    float wavelength=wave.w;
    float k=2.*3.14159/wavelength;
    float c=sqrt(9.8/k);
    vec2 d=normalize(wave.xy);
    float f=k*(dot(d,p.xz)-c*time);
    float a=steepness/k;
    return vec3(d.x*(a*cos(f)), a*sin(f), d.y*(a*cos(f)));
}

void main(){
    UV=uv+vec2(time*.05,time*.03);
    float amp=0.5;

    // Exactly 3 Gerstner waves like reference — different dirs
    vec3 w1=gerstner(vec4(1.0, 0.0,   amp,      50.0), pos);
    vec3 w2=gerstner(vec4(0.707,0.707, amp*0.7,  31.0), pos);
    vec3 w3=gerstner(vec4(0.5, 0.866,  amp*0.5,  18.0), pos);

    vec3 displaced=pos+w1+w2*waveRev+w3*waveRev;
    wH=displaced.y-pos.y;
    dep=length(vec2(pos.x,pos.z))/2000.;

    // Normal from wave tangents (reference method)
    vec3 tang=vec3(1,0,0)+vec3(
        -pow(sin(3.14159*2./50.*(pos.x-sqrt(9.8/(2.*3.14159/50.))*time)),2.)*amp*.1,
        cos(3.14159*2./50.*(pos.x-sqrt(9.8/(2.*3.14159/50.))*time))*amp*.2,
        0);
    vec3 bino=vec3(0,0,1)+vec3(0,
        cos(3.14159*2./31.*(0.707*pos.x+0.707*pos.z-sqrt(9.8/(2.*3.14159/31.))*time))*amp*.14,
        -pow(sin(3.14159*2./31.*(0.707*pos.x+0.707*pos.z-sqrt(9.8/(2.*3.14159/31.))*time)),2.)*amp*.1);
    wNorm=normalize(cross(bino,tang));

    fragPos=displaced;
    vColor=vec4(0.2, 0.4+0.3*sin(wH*5.), 0.8+0.2*cos(wH*3.), 0.7);

    vec3 p=displaced; p.y-=4.5;
    gl_Position=MVP*vec4(p,1);
}
)";
const char* wF = R"(
#version 410 core
in vec2 UV; in float wH,dep;
in vec3 wNorm,fragPos; in vec4 vColor;
uniform float reveal,waveRev,foamRev,time; out vec4 F;
void main(){
    float wn=clamp(wH/2.5+.5,0.,1.);
    float closeF=clamp(1.-dep*4.,0.,1.);

    // Your purple colors
    vec3 deepPurple=vec3(.10,.02,.22);
    vec3 midPurple =vec3(.18,.04,.32);
    vec3 nearPurp  =vec3(.16,.04,.35);
    vec3 baseColor=mix(deepPurple,midPurple,wn*.4);
    baseColor=mix(baseColor,nearPurp,closeF*.35);
    baseColor=mix(baseColor,deepPurple,dep*1.5);
    baseColor=mix(baseColor,vec3(.25,.08,.42),wn*.25);
    baseColor=mix(baseColor,vec3(.06,.01,.14),(1.-wn)*.3);

    // Animated UV scrolling
    vec2 uv1=UV+vec2(time*.03,time*.022);
    vec2 uv2=UV+vec2(-time*.025,time*.030);

    // Rich ripple texture
    float r1=sin(uv1.x*55.+time*4.)*cos(uv1.y*50.+time*3.5)*.06;
    float r2=sin(uv2.x*80.-time*5.)*sin(uv2.y*75.+time*4.5)*.05;
    float r3=cos(UV.x*32.+UV.y*38.+time*2.8)*.05;
    float r4=sin(UV.x*105.+UV.y*95.-time*6.)*.04;
    float r5=cos(UV.x*18.-UV.y*22.+time*1.9)*.05;
    float r6=sin(uv1.x*135.+uv1.y*125.+time*7.)*.04;
    baseColor+=vec3(.4,.12,.58)*(r1+r2+r3+r4+r5+r6)*waveRev;

    // Caustics from reference
    float caus=sin(fragPos.x*2.+time)*cos(fragPos.z*2.-time)*.5+.5;
    caus*=sin(fragPos.x*.5+time*.5)*.3+.5;
    baseColor+=vec3(.28,.08,.45)*caus*.15*closeF;

    // Fresnel from reference
    vec3 viewDir=normalize(vec3(0,10,20)-fragPos);
    float fresnel=pow(1.-abs(dot(viewDir,wNorm)),3.);
    fresnel=mix(.2,1.,fresnel);
    baseColor+=vec3(.15,.04,.28)*fresnel*.2;

    // Specular sun
    vec3 sunDir=normalize(vec3(.4,.5,-1.));
    vec3 halfDir=normalize(sunDir+viewDir);
    float spec=pow(max(dot(wNorm,halfDir),0.),45.)*.8;
    baseColor+=vec3(1.,.85,.3)*spec*waveRev;

    // Foam — shore only
    float shoreFoam=(1.-smoothstep(0.,.08,dep))*(sin(dep*20.+time*3.)*.5+.5)*foamRev;
    float crestFoam=smoothstep(.65,1.,wn)*(1.-smoothstep(0.,.12,dep))*foamRev;
    baseColor=mix(baseColor,vec3(.93,.96,1.),clamp(shoreFoam+crestFoam,0.,1.));

    // Shimmer
    float shimmer=sin(UV.x*14.+time*2.3)*cos(UV.y*12.+time*1.7)*.025;
    baseColor+=vec3(.4,.12,.58)*shimmer*waveRev;

    baseColor=mix(vec3(0),baseColor,reveal);
    float alpha=mix(.88,.97,clamp(dep*3.,0.,1.)); // opaque in depth
    F=vec4(baseColor,alpha*reveal);
}
)";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// SAND SHADER (row 5)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
const char* sdV = R"(
#version 410 core
layout(location=0) in vec3 pos; layout(location=1) in vec2 uv;
uniform mat4 MVP; out vec2 UV; out float sh;
void main(){
    UV=uv; sh=clamp((pos.z+10.)/40.,0.,1.);
    vec3 p=pos; p.y+=sin(pos.x*.3)*cos(pos.z*.25)*.4+1.5;
    gl_Position=MVP*vec4(p,1);
}
)";
const char* sdF = R"(
#version 410 core
in vec2 UV; in float sh; uniform float reveal,time; out vec4 F;

float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5); }

void main(){
    // ── BASE SAND ────────────────────────────────────────────────
    float dune=sin(UV.x*2.5+UV.y*1.8)*.5+.5;
    float dune2=sin(UV.x*4.2-UV.y*3.1+1.3)*.5+.5;
    vec3 wetSand =vec3(.42,.26,.18);
    vec3 drySand =vec3(.75,.58,.36);
    vec3 goldSand=vec3(.82,.65,.40);
    vec3 c=mix(wetSand,drySand,dune);
    c=mix(c,goldSand,dune2*.25);

    // ── RIPPLE MARKS ─────────────────────────────────────────────
    float ripple=sin(UV.x*22.+UV.y*16.)*.025
                +sin(UV.x*35.-UV.y*28.)*.015;
    c+=ripple;

    // ── SAND GRAINS ──────────────────────────────────────────────
    vec2 gUV=UV*200.;
    vec2 gi=floor(gUV); vec2 gf=fract(gUV);
    float grain=0.;
    for(int gy=-1;gy<=1;gy++) for(int gx=-1;gx<=1;gx++){
        vec2 g=gi+vec2(gx,gy);
        vec2 o=vec2(hash(g),hash(g+vec2(13.7,57.3)));
        float d=length(vec2(gx,gy)-gf+o);
        grain+=smoothstep(.4,.0,d)*.07*(mix(.6,1.3,hash(g)));
    }
    c+=grain;

    // ── 3D PEBBLES — sphere/cylinder bump mapping ─────────────────
    vec2 pUV=UV*30.;
    vec2 pi2=floor(pUV);
    vec2 pf=fract(pUV);
    vec3 pebbleCol=c;
    float pebbleBlend=0.;
    vec3 totalNorm=vec3(0,0,1); // accumulated normal

    for(int py=-1;py<=1;py++) for(int px=-1;px<=1;px++){
        vec2 pg=pi2+vec2(px,py);
        vec2 po=vec2(hash(pg)*.75+.125, hash(pg+vec2(7.3,11.5))*.75+.125);
        vec2 pr=vec2(px,py)-pf+po;

        // Random pebble shape: round or elongated
        float hA=hash(pg+vec2(1.1,2.2));
        float hB=hash(pg+vec2(3.3,4.4));
        float hC=hash(pg+vec2(5.5,6.6));
        float hD=hash(pg+vec2(7.7,8.8));

        // Size variation: small to large
        float size=mix(.18,.42,hA);
        // Shape: sphere (round) or cylinder (elongated)
        float elongate=mix(1.,2.5,step(.5,hB)); // 50% chance elongated
        float angle=hC*3.14159;
        vec2 rot=vec2(pr.x*cos(angle)-pr.y*sin(angle),
                      pr.x*sin(angle)+pr.y*cos(angle));
        float pd=length(rot/vec2(elongate,1.))/size;

        if(pd<1.0){
            // 3D sphere normal from position on pebble
            float inside=clamp(1.-pd,0.,1.);
            float zH=sqrt(max(1.-pd*pd,0.)); // hemisphere height

            // Normal vector (sphere-like)
            vec3 pNorm=normalize(vec3(rot.x/size, rot.y/size, zH*1.5));

            // Lighting — sun from upper right
            vec3 sunDir=normalize(vec3(.5,.7,-1.));
            float diffuse=max(dot(pNorm,sunDir),0.);
            float spec=pow(max(dot(reflect(-sunDir,pNorm),vec3(0,0,1)),0.),32.);

            // Pebble color — varied greys, browns, dark stones
            vec3 pc;
            if(hD<.25)      pc=vec3(.32,.28,.25); // dark grey
            else if(hD<.5)  pc=vec3(.52,.44,.35); // brown
            else if(hD<.75) pc=vec3(.42,.38,.34); // mid grey
            else            pc=vec3(.28,.22,.18); // dark brown

            // Wet dark bottom edge
            float wetEdge=smoothstep(.3,1.,pd)*.5;
            pc=mix(pc,pc*.4,wetEdge);

            // Apply lighting
            pc=pc*(diffuse*.7+.3)+spec*.4;

            // Sunset tint on top
            pc=mix(pc,pc*vec3(1.2,.9,.7),pNorm.y*.3);

            float edge=smoothstep(1.,.4,pd);
            pebbleCol=mix(pebbleCol,pc,edge);
            pebbleBlend=max(pebbleBlend,edge);
        }
    }
    c=mix(c,pebbleCol,pebbleBlend);

    // ── 3D SHELLS ──────────────────────────────────────────────────────────
    vec2 shUV=UV*20.; vec2 shi=floor(shUV); vec2 shf=fract(shUV);
    for(int sy=-1;sy<=1;sy++) for(int sx=-1;sx<=1;sx++){
        vec2 sg=shi+vec2(sx,sy);
        float rnd=hash(sg+vec2(77.3,44.1));
        if(rnd>.82){
            vec2 so=vec2(hash(sg)*.8+.1,hash(sg+vec2(5.1,9.3))*.8+.1);
            vec2 sr=vec2(sx,sy)-shf+so;
            float ang=hash(sg+vec2(2.2,5.5))*6.28;
            vec2 rot=vec2(sr.x*cos(ang)-sr.y*sin(ang),sr.x*sin(ang)+sr.y*cos(ang));
            float size=mix(.25,.45,hash(sg+vec2(3.,7.)));
            float sd3=length(rot/vec2(1.6,1.))/size;
            if(sd3<1.0){
                float zH=sqrt(max(1.-sd3*sd3,0.));
                vec3 sNorm=normalize(vec3(rot.x/size,rot.y/size,zH*2.));
                float spiralA=atan(rot.y,rot.x);
                float spiralR=length(rot)/size;
                float groove=sin(spiralA*6.+spiralR*8.)*.5+.5;
                vec3 shellBase=step(.5,hash(sg+vec2(11.,13.)))>.5
                    ? mix(vec3(.92,.85,.72),vec3(.75,.60,.48),groove)
                    : mix(vec3(.85,.72,.80),vec3(.65,.45,.55),groove);
                shellBase=mix(shellBase*.6,shellBase,smoothstep(0.,.3,sd3));
                vec3 sunDir=normalize(vec3(.5,.7,-1.));
                float diff=max(dot(sNorm,sunDir),0.)*.7+.25;
                float spec=pow(max(dot(reflect(-sunDir,sNorm),vec3(0,0,1)),0.),20.)*.5;
                vec3 sc=shellBase*diff+spec;
                sc=mix(sc,c,smoothstep(.7,1.,sd3)*.7);
                c=mix(c,sc,smoothstep(1.,.3,sd3)*.95);
            }
        }
    }

    // ── SUNSET TINT ───────────────────────────────────────────────
    c=mix(c,c*vec3(1.35,.80,.58),sh*.4+.2);
    c=mix(c,vec3(.35,.20,.30),(1.-sh)*.4);
    c=mix(vec3(0),c,reveal);
    F=vec4(c,1);
}
)";


// ── MOUNTAIN SHADER ───────────────────────────────────────────────────────────
const char* mtnV = R"(
#version 410 core
layout(location=0) in vec3 pos; layout(location=1) in vec2 uv;
uniform mat4 MVP; out vec2 UV; out float ht;
void main(){
    UV=uv;
    // Create mountain peaks using sine waves
    float peak = sin(pos.x*0.06)*28.+sin(pos.x*0.11+1.)*18.+sin(pos.x*0.04+2.)*12.
                +cos(pos.z*0.05)*8.+sin(pos.x*0.20)*6.;
    ht=clamp(peak/25.,0.,1.);
    vec3 p=pos; p.y+=peak;
    gl_Position=MVP*vec4(p,1);
}
)";
const char* mtnF = R"(
#version 410 core
in vec2 UV; in float ht;
uniform float reveal; out vec4 F;
void main(){
    // Rocky mountain colors: dark grey at base, lighter at peaks
    // Dark silhouette like in the photo — very dark purple-black
    vec3 silhouette=vec3(0.06,0.03,0.10);
    vec3 litEdge   =vec3(0.15,0.08,0.20);
    vec3 col=mix(silhouette,litEdge,ht*0.3);
    // Slight orange glow on mountain peaks from sunset
    col=mix(col,vec3(0.25,0.10,0.05),smoothstep(0.6,1.0,ht)*0.4);
    col=mix(vec3(0),col,reveal);
    F=vec4(col,1);
}
)";


// ── PALM TRUNK SHADER ─────────────────────────────────────────────────────────
const char* trunkV = R"(
#version 410 core
layout(location=0) in vec3 pos;
layout(location=1) in vec2 uv;
uniform mat4 MVP;
out vec3 wP; out vec2 UV; out vec3 N;
void main(){
    wP=pos; UV=uv;
    // Normal from cylinder
    N=normalize(vec3(pos.x,0,pos.z));
    gl_Position=MVP*vec4(pos,1);
}
)";
const char* trunkF = R"(
#version 410 core
in vec3 wP; in vec2 UV; in vec3 N;
uniform float reveal; out vec4 F;
void main(){
    // Bark rings and texture
    float ring=sin(wP.y*6.)*.4+.6;
    float grain=sin(UV.x*40.+wP.y*3.)*.05+sin(UV.x*80.)*.03;
    vec3 dark =vec3(.18,.09,.03);
    vec3 mid  =vec3(.32,.18,.07);
    vec3 light=vec3(.45,.28,.12);
    vec3 col=mix(dark,mid,ring)+grain;
    // Lighting — sun from front-right
    vec3 sunDir=normalize(vec3(1,.5,-1));
    float diff=max(dot(N,sunDir),0.)*.5+.35;
    // Warm sunset tint
    col=col*diff;
    col=mix(col,col*vec3(1.3,.8,.5),.3);
    col=mix(vec3(0),col,reveal);
    F=vec4(col,1);
}
)";

// ── PALM LEAF SHADER ──────────────────────────────────────────────────────────
const char* leafV = R"(
#version 410 core
layout(location=0) in vec3 pos;
layout(location=1) in vec2 uv;
uniform mat4 MVP; uniform float time; uniform int leafIdx;
out vec2 UV; out float NdotL;
void main(){
    UV=uv;
    // Each leaf sways differently
    float phase=float(leafIdx)*0.9;
    // Multiple wind frequencies for organic feel
    float spd=3.5; // faster wind
    float gust   = sin(time*spd*0.8+phase)*0.9
                 + sin(time*spd*1.7+phase*1.2)*0.50
                 + sin(time*spd*3.1+phase*0.5)*0.25
                 + sin(time*spd*5.0+phase*0.3)*0.15;
    float gustZ  = cos(time*spd*0.9+phase)*0.60
                 + cos(time*spd*2.1+phase*0.8)*0.35
                 + cos(time*spd*4.2+phase*1.1)*0.18;
    // Wind increases from base to tip (uv.y=0 base, 1 tip)
    float swFactor=uv.y*uv.y*uv.y; // cubic — even more movement at tip
    float sw =gust *swFactor*1.8;
    float swz=gustZ*swFactor*1.2;
    vec3 p=pos;
    p.x+=sw; p.z+=swz;
    // Natural droop under gravity — more at tip
    p.y-=uv.y*uv.y*3.0;
    // Lighting normal (simplified)
    NdotL=max(dot(normalize(vec3(sw,1,swz)),normalize(vec3(1,.5,-1))),0.)*.5+.4;
    gl_Position=MVP*vec4(p,1);
}
)";
const char* leafF = R"(
#version 410 core
in vec2 UV; in float NdotL;
uniform float reveal; out vec4 F;
void main(){
    // Leaf shape — pointed tip, tapers at edges
    float edgeFade=1.-pow(abs(UV.x-.5)*2.,2.5);
    float tipFade =1.-UV.y;
    float shape=edgeFade*pow(tipFade,.3);
    if(shape<.05) discard;
    // Leaf color — darker vein in centre
    float vein=1.-pow(abs(UV.x-.5)*2.,.5)*.4;
    vec3 lightG=vec3(.15,.55,.08);
    vec3 darkG =vec3(.05,.28,.04);
    vec3 col=mix(darkG,lightG,vein);
    // Sunset tint on top surface
    col=mix(col,col*vec3(1.1,.85,.6),.3);
    col=col*NdotL;
    col=mix(vec3(0),col,reveal*shape);
    F=vec4(col,reveal*shape);
}
)";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// HELPERS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
GLuint makeShader(const char* v,const char* f){
    auto c=[](GLenum t,const char* s){
        GLuint sh=glCreateShader(t); glShaderSource(sh,1,&s,0); glCompileShader(sh);
        GLint ok; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
        if(!ok){char b[512];glGetShaderInfoLog(sh,512,0,b);std::cerr<<b<<"\n";}
        return sh;
    };
    GLuint p=glCreateProgram();
    glAttachShader(p,c(GL_VERTEX_SHADER,v)); glAttachShader(p,c(GL_FRAGMENT_SHADER,f));
    glLinkProgram(p); return p;
}

GLuint loadTex(const char* path){
    GLuint id; glGenTextures(1,&id);
    int w,h,ch; stbi_set_flip_vertically_on_load(true);
    auto* d=stbi_load(path,&w,&h,&ch,3);
    if(!d){std::cerr<<"Tex failed: "<<path<<"\n"; return 0;}
    glBindTexture(GL_TEXTURE_2D,id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,d);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    stbi_image_free(d);
    std::cout<<"Tex: "<<path<<" "<<w<<"x"<<h<<"\n"; return id;
}

int gSphereIdx=0;
GLuint makeSphere(){
    const int ST=24,SL=24;
    std::vector<float> v; std::vector<unsigned> idx;
    for(int i=0;i<=ST;i++){float phi=M_PI*i/ST;
        for(int j=0;j<=SL;j++){float th=2*M_PI*j/SL;
            v.push_back(sinf(phi)*cosf(th)); v.push_back(cosf(phi)); v.push_back(sinf(phi)*sinf(th));}}
    for(int i=0;i<ST;i++) for(int j=0;j<SL;j++){unsigned a=i*(SL+1)+j;
        idx.push_back(a);idx.push_back(a+SL+1);idx.push_back(a+1);
        idx.push_back(a+1);idx.push_back(a+SL+1);idx.push_back(a+SL+2);}
    gSphereIdx=(int)idx.size();
    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,v.size()*4,v.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*4,idx.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,(void*)0);
    glBindVertexArray(0); return vao;
}

GLuint makeGrid(float W,float D,int R,float Y){
    std::vector<float> v; std::vector<unsigned> idx;
    for(int zi=0;zi<=R;zi++) for(int xi=0;xi<=R;xi++){
        float px=-W*.5f+W*xi/R, pz=-D*.5f+D*zi/R;
        v.push_back(px);v.push_back(Y);v.push_back(pz);
        v.push_back((float)xi/R*8);v.push_back((float)zi/R*8);}
    for(int zi=0;zi<R;zi++) for(int xi=0;xi<R;xi++){unsigned a=zi*(R+1)+xi;
        idx.push_back(a);idx.push_back(a+R+1);idx.push_back(a+1);
        idx.push_back(a+1);idx.push_back(a+R+1);idx.push_back(a+R+2);}
    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,v.size()*4,v.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*4,idx.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,20,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,20,(void*)12);
    glBindVertexArray(0); return vao;
}


GLuint makeCylinder(float r, float h, int segs){
    // Realistic palm trunk: tapers from base to top, slight curve
    std::vector<float> v; std::vector<unsigned> idx;
    int VSTACKS=20; // vertical subdivisions for smooth curve
    for(int s=0;s<=VSTACKS;s++){
        float fy=(float)s/VSTACKS;
        float py=fy*h;
        // Taper: thick at base, thinner at top
        float rad=r*(1.0f-fy*.45f);
        // Slight S-curve lean
        float lean=sinf(fy*M_PI*.8f)*1.5f;
        for(int i=0;i<=segs;i++){
            float a=2*M_PI*i/segs;
            float x=cosf(a)*rad+lean*.3f;
            float z=sinf(a)*rad;
            v.push_back(x);v.push_back(py);v.push_back(z);
            v.push_back((float)i/segs);v.push_back(fy);
        }
    }
    for(int s=0;s<VSTACKS;s++) for(int i=0;i<segs;i++){
        unsigned a=s*(segs+1)+i;
        unsigned b=a+1, c=a+segs+1, d=c+1;
        idx.push_back(a);idx.push_back(c);idx.push_back(b);
        idx.push_back(b);idx.push_back(c);idx.push_back(d);
    }
    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,v.size()*4,v.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*4,idx.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,20,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,20,(void*)12);
    glBindVertexArray(0); return vao;
}

// Realistic palm frond — subdivided strip that curves downward
GLuint makeLeaf(float len, float w){
    int segs=20;
    std::vector<float> v;
    std::vector<unsigned> idx;
    for(int i=0;i<=segs;i++){
        float fy=(float)i/segs;
        float py=fy*len*.4f;
        float pz=fy*len;
        // Taper width toward tip
        float ww=w*(1.f-fy*.85f);
        // Left edge
        v.push_back(-ww);v.push_back(py);v.push_back(pz);
        v.push_back(0.f);v.push_back(fy);
        // Right edge
        v.push_back( ww);v.push_back(py);v.push_back(pz);
        v.push_back(1.f);v.push_back(fy);
    }
    for(int i=0;i<segs;i++){
        unsigned b=i*2;
        idx.push_back(b);idx.push_back(b+2);idx.push_back(b+1);
        idx.push_back(b+1);idx.push_back(b+2);idx.push_back(b+3);
    }
    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,v.size()*4,v.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*4,idx.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,20,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,20,(void*)12);
    glBindVertexArray(0); return vao;
}

GLuint makeCube(float hw,float hh,float hd,glm::vec2 u0,glm::vec2 u1){
    float x0=u0.x,y0=u0.y,x1=u1.x,y1=u1.y;
    float v[]={
        // FRONT (z+) — image slice, correct orientation
        -hw,-hh, hd,x0,y0,0,0,1,  hw,-hh, hd,x1,y0,0,0,1,
         hw, hh, hd,x1,y1,0,0,1, -hw, hh, hd,x0,y1,0,0,1,
        // BACK (z-) — columns inverted
         hw,-hh,-hd,x1,y0,0,0,-1,-hw,-hh,-hd,x0,y0,0,0,-1,
        -hw, hh,-hd,x0,y1,0,0,-1, hw, hh,-hd,x1,y1,0,0,-1,
        // LEFT
        -hw,-hh,-hd,x0,y0,-1,0,0,-hw,-hh, hd,x1,y0,-1,0,0,
        -hw, hh, hd,x1,y1,-1,0,0,-hw, hh,-hd,x0,y1,-1,0,0,
        // RIGHT
         hw,-hh, hd,x0,y0,1,0,0, hw,-hh,-hd,x1,y0,1,0,0,
         hw, hh,-hd,x1,y1,1,0,0, hw, hh, hd,x0,y1,1,0,0,
        // TOP
        -hw, hh, hd,x0,y0,0,1,0,  hw, hh, hd,x1,y0,0,1,0,
         hw, hh,-hd,x1,y1,0,1,0, -hw, hh,-hd,x0,y1,0,1,0,
        // BOTTOM
        -hw,-hh,-hd,x0,y0,0,-1,0, hw,-hh,-hd,x1,y0,0,-1,0,
         hw,-hh, hd,x1,y1,0,-1,0,-hw,-hh, hd,x0,y1,0,-1,0,
    };
    unsigned idx[36]; for(int f=0;f<6;f++){unsigned b=f*4;
        idx[f*6]=b;idx[f*6+1]=b+1;idx[f*6+2]=b+2;
        idx[f*6+3]=b;idx[f*6+4]=b+2;idx[f*6+5]=b+3;}
    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    int s=8*sizeof(float);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,s,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,s,(void*)12);
    glEnableVertexAttribArray(2);glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,s,(void*)20);
    glBindVertexArray(0); return vao;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// STRUCTS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
struct Piece{GLuint vao=0;glm::vec3 cur,target,snapStart;float snapT=-1,bob=0;bool placed=false;};
struct Orb{glm::vec3 pos,base,color;float r,ps,ds,dr;};

double gScroll=0;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// MAIN
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    GLFWwindow* win=glfwCreateWindow(1280,720,"CHROMATIC ECHOES",0,0);
    glfwMakeContextCurrent(win);
    int fw,fh; glfwGetFramebufferSize(win,&fw,&fh);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0,0,fw,fh);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(win,[](GLFWwindow*,double,double dy){gScroll+=dy;});

    // Shaders
    GLuint pSh=makeShader(pV,pF);
    GLuint oSh=makeShader(oV,oF);
    GLuint rSh=makeShader(rV,rF);
    GLuint sSh=makeShader(sV,sF);
    GLuint skySh=makeShader(skyV,skyF);
    GLuint shootSh=makeShader(shootV,shootF);
    GLuint watSh=makeShader(wV,wF);
    GLuint sndSh=makeShader(sdV,sdF);
    // Palm tree
    GLuint trunkSh=makeShader(trunkV,trunkF);
    GLuint leafSh =makeShader(leafV, leafF);
    // Tall, thick trunk
    GLuint trunkVAO=makeCylinder(0.7f,22.f,16);
    int trunkIdx=20*16*6;
    // Long wide fronds
    GLuint leafVAO=makeLeaf(12.f,1.2f);
    int leafIdx=20*2*3;



    GLuint texID=loadTex("assets/textures/puzzle_image.jpg");
    GLuint sphereVAO=makeSphere();
    GLuint skyVAO=makeSphere();

    // Beach geometry
    int wIdx=64*64*6; GLuint watVAO=makeGrid(2000000,2000000,64,0);
    int sIdx=32*32*6; GLuint sndVAO=makeGrid(400,300,32,0);

    // ── PUZZLE 6×5 = 30 pieces ────────────────────────────────────────────────
    const int C=6,R=5; const float PW=2.8f,PH=2.8f,PD=0.5f;
    float tW=C*PW,tH=R*PH;
    std::vector<Piece> pieces; srand(42);
    for(int r=0;r<R;r++) for(int c=0;c<C;c++){
        float tx=-tW*.5f+c*PW+PW*.5f, ty=tH*.5f-r*PH-PH*.5f;
        glm::vec2 u0((float)c/C,1.f-(float)(r+1)/R);
        glm::vec2 u1((float)(c+1)/C,1.f-(float)r/R);
        Piece p; p.target=glm::vec3(tx,ty,0);
        p.cur=p.snapStart=glm::vec3((rand()%260-130)*.1f,(rand()%160-80)*.1f,(rand()%80)*.1f+2.f);
        p.bob=(float)(r*C+c)*.53f;
        p.vao=makeCube(PW*.5f,PH*.5f,PD*.5f,u0,u1);
        pieces.push_back(p);
    }

    // ── ORBS 50 total ─────────────────────────────────────────────────────────
    std::vector<glm::vec3> pal={
        {.8f,0,1},{1,.1f,.4f},{0,.6f,1},{1,.5f,0},{.2f,1,.4f},
        {1,.9f,0},{.9f,0,.5f},{0,.9f,.8f},{1,.2f,.2f},{.5f,0,1},{1,.3f,.8f},{0,1,.6f}
    };
    std::vector<Orb> orbs;
    struct OD{glm::vec3 p;int c;float r,ps,ds,dr;};
    for(auto& d:std::vector<OD>{
        {{-35,12,-80},0,25,.7f,.06f,6},{{45,-10,-90},1,22,1.1f,.05f,7},
        {{8,30,-70},2,28,.5f,.04f,8}, {{-60,-5,-60},3,20,1.3f,.08f,5},
        {{35,25,-65},4,22,.9f,.06f,6},{{-25,-25,-75},5,20,.6f,.07f,7},
        {{65,12,-50},6,24,.8f,.05f,8},{{0,-40,-80},7,22,1,.06f,7},
        {{-55,35,-55},8,18,1.2f,.09f,5},{{50,-20,-60},9,20,.75f,.07f,6},
        {{-10,45,-70},10,26,.55f,.04f,9},{{30,-35,-65},11,19,1,.08f,5},
        {{-15,8,-25},0,8,1.5f,.20f,3},{{18,-5,-22},2,7,2,.25f,3},
        {{-8,15,-30},4,9,1.3f,.18f,4},{{22,8,-18},1,7,1.8f,.22f,2},
        {{-28,-8,-28},5,8,1.6f,.19f,3},{{14,18,-35},3,8,2.2f,.24f,3},
        {{-5,-18,-20},6,7,1.4f,.21f,2.5f},{{28,-12,-25},7,9,1.7f,.18f,3},
        {{-35,5,-30},8,8,1.9f,.15f,4},{{10,28,-28},9,7,1.3f,.22f,3},
        {{-20,20,-22},10,9,1.6f,.20f,3},{{35,0,-20},11,7,2,.23f,2.5f},
        {{-12,4,-10},0,5,2.5f,.40f,1.5f},{{9,-3,-8},2,4,3,.45f,1},
        {{-5,12,-12},4,5.5f,2.2f,.35f,1.5f},{{15,6,-10},6,4.5f,2.8f,.42f,1},
        {{-20,9,-14},1,4,3.2f,.38f,1.3f},{{7,-9,-8},7,5,2.4f,.36f,1.2f},
        {{3,15,-15},3,5,2.6f,.40f,1.4f},{{-8,-8,-6},5,4,3,.45f,1},
        {{18,-4,-12},8,5,2.3f,.38f,1.2f},{{-15,-5,-8},9,4.5f,2.7f,.42f,1},
        {{0,8,-6},10,4,3.5f,.50f,.8f},{{12,12,-10},11,5,2.9f,.40f,1},
        {{-3,2,-4},0,1.5f,4,.60f,.5f},{{5,4,-5},3,1.2f,5,.55f,.4f},
        {{-6,-2,-3},6,1.8f,3.5f,.58f,.6f},{{2,-5,-4},9,1,6,.65f,.3f},
        {{4,6,-6},2,1.4f,4.5f,.62f,.5f},{{-4,5,-5},7,1.3f,5,.60f,.4f},
        {{-22,0,-18},3,6,1.7f,.17f,2.5f},{{16,-14,-20},5,5.5f,1.9f,.21f,2},
        {{-30,15,-35},0,7,1.4f,.14f,3},{{25,15,-32},1,6.5f,1.6f,.16f,2.5f},
        {{-12,-15,-24},4,6,2.1f,.22f,2},{{8,20,-26},6,7,1.5f,.15f,3},
    }){
        Orb o; o.base=o.pos=d.p; o.color=pal[d.c%12];
        o.r=d.r; o.ps=d.ps; o.ds=d.ds; o.dr=d.dr; orbs.push_back(o);
    }

    // Rays
    std::vector<std::pair<int,int>> rays={
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,9},{9,10},{10,11},{11,0},
        {0,4},{1,5},{2,6},{3,7},{4,8},{5,9},
        {12,0},{13,1},{14,2},{15,3},{16,4},{17,5},{18,6},{19,7},
        {12,13},{13,14},{14,15},{15,16},{16,17},{17,18},{18,19},{19,20},{20,21},{21,22},{22,23},{23,12},
        {24,12},{25,13},{26,14},{27,15},{28,16},{29,17},{30,18},{31,19},
        {24,25},{25,26},{26,27},{27,28},{28,29},{29,30},{30,31},{31,24},
    };
    GLuint lineVAO,lineVBO;
    glGenVertexArrays(1,&lineVAO);glGenBuffers(1,&lineVBO);
    glBindVertexArray(lineVAO);glBindBuffer(GL_ARRAY_BUFFER,lineVBO);
    glBufferData(GL_ARRAY_BUFFER,5*4*sizeof(float),nullptr,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,20,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,20,(void*)12);
    glBindVertexArray(0);

    // Stars
    std::vector<float> starD; srand(1337);
    for(int i=0;i<2000;i++){
        float ph=acosf(1-2*(rand()/(float)RAND_MAX));
        float th=2*M_PI*(rand()/(float)RAND_MAX); float r=80+rand()%40;
        starD.push_back(r*sinf(ph)*cosf(th));starD.push_back(r*sinf(ph)*sinf(th));
        starD.push_back(r*cosf(ph));starD.push_back(.3f+.7f*(rand()/(float)RAND_MAX));
    }
    GLuint starVAO,starVBO;
    glGenVertexArrays(1,&starVAO);glGenBuffers(1,&starVBO);
    glBindVertexArray(starVAO);glBindBuffer(GL_ARRAY_BUFFER,starVBO);
    glBufferData(GL_ARRAY_BUFFER,starD.size()*4,starD.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,16,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,16,(void*)12);
    glBindVertexArray(0);

    // Camera
    glm::vec3 camPos(0,2,50),camFront(glm::normalize(glm::vec3(0,-0.05f,-1))),camUp(0,1,0);
    float yaw=-90,pitch=-3,lastX=fw*.5f,lastY=fh*.5f; bool firstM=true;
    float fov=45,pDist=18;
    glm::mat4 proj=glm::perspective(glm::radians(fov),(float)fw/fh,.1f,100000.f);
    glm::mat4 puzView=glm::lookAt(glm::vec3(0,1,pDist),glm::vec3(0,0,0),glm::vec3(0,1,0));

    // Puzzle rotation
    float rotX=0,rotY=0; bool autoSpin=true;
    double rmLX=0,rmLY=0; bool rmWas=false;

    int nxt=0; float t=0; bool pWas=false,mWas=false;


    // Shooting stars — 15 trails across the sky
    std::vector<float> shootD;
    srand(999);
    for(int i=0;i<30;i++){
        float x=(rand()%200-100)*.8f;
        float y=20.f+(rand()%40)*.8f;
        float z=-50.f-(rand()%50)*.5f;
        float phase=rand()/(float)RAND_MAX;
        shootD.push_back(x);shootD.push_back(y);shootD.push_back(z);
        shootD.push_back(phase);
    }
    GLuint shootVAO,shootVBO;
    glGenVertexArrays(1,&shootVAO);glGenBuffers(1,&shootVBO);
    glBindVertexArray(shootVAO);glBindBuffer(GL_ARRAY_BUFFER,shootVBO);
    glBufferData(GL_ARRAY_BUFFER,shootD.size()*4,shootD.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,16,(void*)0);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,16,(void*)12);
    glBindVertexArray(0);
    // Thresholds: 1 per row of 6 pieces
    const int T1=6,T2=12,T3=18,T4=24,T5=30;

    std::cout<<"WASD=fly  Mouse=look  Scroll=zoom  P/Click=place\n";
    std::cout<<"J/L=rotate puzzle  I/K=tilt  RightClick+drag=rotate  R=reset\n";

    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();
        if(glfwGetKey(win,GLFW_KEY_ESCAPE)==GLFW_PRESS) break;
        t+=.016f;

        // Mouse look
        double mx,my; glfwGetCursorPos(win,&mx,&my);
        if(firstM){lastX=(float)mx;lastY=(float)my;firstM=false;}
        yaw+=(float)(mx-lastX)*.08f; pitch=glm::clamp(pitch-(float)(my-lastY)*.08f,-89.f,89.f);
        lastX=(float)mx; lastY=(float)my;
        camFront=glm::normalize(glm::vec3(
            cosf(glm::radians(yaw))*cosf(glm::radians(pitch)),
            sinf(glm::radians(pitch)),
            sinf(glm::radians(yaw))*cosf(glm::radians(pitch))));

        // Movement
        float sp=0.8f; glm::vec3 right=glm::normalize(glm::cross(camFront,camUp));
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS||glfwGetKey(win,GLFW_KEY_UP)==GLFW_PRESS)    camPos+=camFront*sp;
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS||glfwGetKey(win,GLFW_KEY_DOWN)==GLFW_PRESS)  camPos-=camFront*sp;
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS||glfwGetKey(win,GLFW_KEY_LEFT)==GLFW_PRESS)  camPos-=right*sp;
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS||glfwGetKey(win,GLFW_KEY_RIGHT)==GLFW_PRESS) camPos+=right*sp;
        if(glfwGetKey(win,GLFW_KEY_SPACE)==GLFW_PRESS)      camPos.y+=sp;
        if(glfwGetKey(win,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) camPos.y-=sp;

        // Puzzle rotation
        if(glfwGetKey(win,GLFW_KEY_J)==GLFW_PRESS){rotY-=1.2f;autoSpin=false;}
        if(glfwGetKey(win,GLFW_KEY_L)==GLFW_PRESS){rotY+=1.2f;autoSpin=false;}
        if(glfwGetKey(win,GLFW_KEY_I)==GLFW_PRESS){rotX-=1.2f;autoSpin=false;}
        if(glfwGetKey(win,GLFW_KEY_K)==GLFW_PRESS){rotX+=1.2f;autoSpin=false;}
        if(glfwGetKey(win,GLFW_KEY_R)==GLFW_PRESS){rotX=0;rotY=0;autoSpin=true;}
        bool rmN=glfwGetMouseButton(win,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;
        if(rmN){double rx,ry;glfwGetCursorPos(win,&rx,&ry);
            if(rmWas){rotY+=(float)(rx-rmLX)*.4f;rotX+=(float)(ry-rmLY)*.4f;autoSpin=false;}
            rmLX=rx;rmLY=ry;} rmWas=rmN;
        if(autoSpin) rotY+=.15f;

        // Scroll zoom
        if(gScroll!=0){
            fov=glm::clamp(fov-(float)gScroll*3.f,15.f,90.f);
            pDist=glm::clamp(pDist-(float)gScroll*.8f,6.f,40.f);
            proj=glm::perspective(glm::radians(fov),(float)fw/fh,.1f,300.f);
            puzView=glm::lookAt(glm::vec3(0,1,pDist),glm::vec3(0,0,0),glm::vec3(0,1,0));
            gScroll=0;
        }

        // Place piece
        bool pN=glfwGetKey(win,GLFW_KEY_P)==GLFW_PRESS;
        bool mN=glfwGetMouseButton(win,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
        if((pN&&!pWas)||(mN&&!mWas)){
            if(nxt<(int)pieces.size()){
                pieces[nxt].snapStart=pieces[nxt].cur; pieces[nxt].snapT=0;
                std::cout<<"["<<++nxt<<"/"<<pieces.size()<<"]\n";
            }
        } pWas=pN; mWas=mN;

        // Update orbs
        for(auto& o:orbs) o.pos=o.base+glm::vec3(
            sinf(t*o.ds+o.base.x)*o.dr,cosf(t*o.ds*.7f+o.base.y)*o.dr*.6f,
            sinf(t*o.ds*.9f+o.base.z)*o.dr*.8f);

        // Update pieces
        for(int i=0;i<(int)pieces.size();i++){
            auto& p=pieces[i]; if(p.placed) continue;
            if(p.snapT>=0){
                p.snapT+=.016f; float st=glm::clamp(p.snapT/.6f,0.f,1.f);
                float e=st*st*(3-2*st);
                glm::vec3 mid=(p.snapStart+p.target)*.5f+glm::vec3(0,3,3);
                p.cur=glm::mix(glm::mix(p.snapStart,mid,e),glm::mix(mid,p.target,e),e);
                if(st>=1.f){p.cur=p.target;p.placed=true;}
            } else { p.cur=p.snapStart; p.cur.y+=sinf(t*1.1f+p.bob)*.12f; }
        }

        glm::mat4 view=glm::lookAt(camPos,camPos+camFront,camUp);

        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // RENDER
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        glViewport(0,0,fw,fh);
        glClearColor(0.08f,0.02f,0.16f,1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        float rev1=glm::clamp((nxt-(float)T1)/3.f,0.f,1.f); // sky
        float rev2=glm::clamp((nxt-(float)T2)/3.f,0.f,1.f); // ocean
        float rev3=glm::clamp((nxt-(float)T3)/3.f,0.f,1.f); // waves
        float rev4=glm::clamp((nxt-(float)T4)/3.f,0.f,1.f); // foam
        float rev5=glm::clamp((nxt-(float)T4)/3.f,0.f,1.f); // sand at row 4

        // ── ROW 1: SKY DOME ──────────────────────────────────────────────────
        if(rev1>0){
            glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);glDisable(GL_DEPTH_TEST);
            // Sky always centered on camera (strip translation from view)
            glm::mat4 skyView=glm::mat4(glm::mat3(view));
            glm::mat4 sm=proj*skyView*glm::scale(glm::mat4(1),glm::vec3(10000.f));
            glUseProgram(skySh);
            glUniformMatrix4fv(glGetUniformLocation(skySh,"MVP"),1,GL_FALSE,glm::value_ptr(sm));
            glUniform1f(glGetUniformLocation(skySh,"reveal"), rev1);
            glUniform1f(glGetUniformLocation(skySh,"reveal2"),rev2);
            glUniform1f(glGetUniformLocation(skySh,"reveal3"),rev3);
            glUniform1f(glGetUniformLocation(skySh,"time"),  t);
            glBindVertexArray(skyVAO);
            glDrawElements(GL_TRIANGLES,gSphereIdx,GL_UNSIGNED_INT,0);
            glEnable(GL_CULL_FACE);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);
        }

        // ── STARS (always visible, fade behind sky) ───────────────────────────
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glDepthMask(GL_FALSE);glDisable(GL_DEPTH_TEST);
        if(rev1>0.05f && camPos.y>-3.f){glm::mat4 VP2=proj*glm::mat4(glm::mat3(view));
         glUseProgram(sSh);
         glUniformMatrix4fv(glGetUniformLocation(sSh,"VP"),1,GL_FALSE,glm::value_ptr(VP2));
         glUniform1f(glGetUniformLocation(sSh,"time"),t);
         glBindVertexArray(starVAO);glDrawArrays(GL_POINTS,0,2000);}
        if(rev1>0.1f && camPos.y>-3.f){
        {glm::mat4 VP=proj*glm::mat4(glm::mat3(view));
         glUseProgram(shootSh);
         glUniformMatrix4fv(glGetUniformLocation(shootSh,"VP"),1,GL_FALSE,glm::value_ptr(VP));
         glUniform1f(glGetUniformLocation(shootSh,"time"),t);
         glBindVertexArray(shootVAO);
         glDrawArrays(GL_POINTS,0,30);}
        } // end stars

        // ── RAYS ──────────────────────────────────────────────────────────────
        glEnable(GL_DEPTH_TEST);
        {glm::mat4 VP=proj*view;
         glUseProgram(rSh);
         glUniformMatrix4fv(glGetUniformLocation(rSh,"VP"),1,GL_FALSE,glm::value_ptr(VP));
         glUniform1f(glGetUniformLocation(rSh,"time"),t);
         glBindVertexArray(lineVAO);glBindBuffer(GL_ARRAY_BUFFER,lineVBO);
         glm::vec3 cr=glm::normalize(glm::cross(camFront,camUp));
         static GLuint bEBO=0;
         if(!bEBO){glGenBuffers(1,&bEBO);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bEBO);
             unsigned bi[]={0,1,2,0,2,3};glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(bi),bi,GL_STATIC_DRAW);}
         else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bEBO);
         for(auto& [a,b]:rays){
             if(a>=(int)orbs.size()||b>=(int)orbs.size()) continue;
             glm::vec3 A=orbs[a].pos,B=orbs[b].pos,side=cr*.8f;
             float bv[]={A.x-side.x,A.y-side.y,A.z-side.z,0,-1,
                         A.x+side.x,A.y+side.y,A.z+side.z,0,1,
                         B.x+side.x,B.y+side.y,B.z+side.z,1,1,
                         B.x-side.x,B.y-side.y,B.z-side.z,1,-1};
             glBufferData(GL_ARRAY_BUFFER,sizeof(bv),bv,GL_DYNAMIC_DRAW);
             glm::vec3 rc=(orbs[a].color+orbs[b].color)*.5f;
             glUniform3f(glGetUniformLocation(rSh,"color"),rc.r,rc.g,rc.b);
             float rayFade=1.f-glm::clamp(rev1,0.f,1.f);
             glUniform1f(glGetUniformLocation(rSh,"alpha"),(.25f+.15f*sinf(t*1.5f+a))*rayFade);
             glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
         }}

        // ── ORBS ──────────────────────────────────────────────────────────────
        glDisable(GL_CULL_FACE);
        {glUseProgram(oSh);glUniform1f(glGetUniformLocation(oSh,"time"),t);
         glBindVertexArray(sphereVAO);
         for(auto& o:orbs){
             glm::mat4 m=glm::scale(glm::translate(glm::mat4(1),o.pos),glm::vec3(o.r));
             glm::mat4 MVP=proj*view*m;
             glUniformMatrix4fv(glGetUniformLocation(oSh,"MVP"),1,GL_FALSE,glm::value_ptr(MVP));
             float orbFade=1.f-glm::clamp(rev1,0.f,1.f);
             glm::vec3 fc=o.color*orbFade;
             glUniform3f(glGetUniformLocation(oSh,"color"),fc.r,fc.g,fc.b);
             glUniform1f(glGetUniformLocation(oSh,"ps"),o.ps);
             glDrawElements(GL_TRIANGLES,gSphereIdx,GL_UNSIGNED_INT,0);
         }}
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_CULL_FACE);




        // ── PALM TREE ─────────────────────────────────────────────────────────
        if(rev5>0){
            glDisable(GL_CULL_FACE);
            // Trunk — slightly curved, leaning toward ocean
            glm::mat4 tBase=glm::translate(glm::mat4(1),glm::vec3(30,-4.f,40))
                           *glm::rotate(glm::mat4(1),glm::radians(-12.f),glm::vec3(0,0,1));
            glUseProgram(trunkSh);
            glUniformMatrix4fv(glGetUniformLocation(trunkSh,"MVP"),1,GL_FALSE,
                glm::value_ptr(proj*view*tBase));
            glUniform1f(glGetUniformLocation(trunkSh,"reveal"),rev5);
            glBindVertexArray(trunkVAO);
            glDrawElements(GL_TRIANGLES,trunkIdx,GL_UNSIGNED_INT,0);

            // Exact trunk top: transform (0,22,0) by tBase matrix
            glm::vec4 tipW = tBase * glm::vec4(0.f, 22.f, 0.f, 1.f);
            glm::vec3 topPos = glm::vec3(tipW);
            glUseProgram(leafSh);
            glUniform1f(glGetUniformLocation(leafSh,"time"),t);
            glUniform1f(glGetUniformLocation(leafSh,"reveal"),rev5);
            glBindVertexArray(leafVAO);
            // 12 fronds spreading in all directions
            for(int li=0;li<12;li++){
                float ang=li*30.f + 15.f;
                float tilt=35.f+sinf(li*1.1f)*20.f; // varied droop
                float scale=0.8f+sinf(li*0.7f)*0.2f; // varied length
                // Leaf base at trunk top, fanning outward and slightly up then drooping
                glm::mat4 lm=glm::translate(glm::mat4(1),topPos)
                    *glm::rotate(glm::mat4(1),glm::radians(ang),  glm::vec3(0,1,0))
                    *glm::rotate(glm::mat4(1),glm::radians(20.f), glm::vec3(1,0,0))
                    *glm::rotate(glm::mat4(1),glm::radians(-tilt),glm::vec3(0,0,1))
                    *glm::scale(glm::mat4(1),glm::vec3(scale,scale,scale));
                glUniformMatrix4fv(glGetUniformLocation(leafSh,"MVP"),1,GL_FALSE,
                    glm::value_ptr(proj*view*lm));
                glUniform1i(glGetUniformLocation(leafSh,"leafIdx"),li);
                glDrawElements(GL_TRIANGLES,leafIdx,GL_UNSIGNED_INT,0);
            }
            glEnable(GL_CULL_FACE);

            // ── Extra trees ───────────────────────────────────────────────
            struct TreeDef{ glm::vec3 pos; float lean; float scale; };
            std::vector<TreeDef> extraTrees={
                {{15,-4.f,35},   -8.f,  0.85f},
                {{45,-4.f,45},  -15.f,  1.0f},
                {{-20,-4.f,60}, -10.f,  0.75f},
                {{60,-4.f,70},  -18.f,  0.90f},
                {{5,-4.f,80},   -12.f,  0.70f},
                {{-40,-4.f,50}, -8.f,   0.95f},
                {{75,-4.f,55},  -20.f,  0.80f},
                {{-10,-4.f,90}, -14.f,  0.88f},
                {{25,-4.f,100}, -9.f,   0.72f},
                {{-55,-4.f,75}, -11.f,  1.05f},
                {{90,-4.f,40},  -16.f,  0.78f},
                {{-30,-4.f,110},-13.f,  0.82f},
                {{50,-4.f,95},  -17.f,  0.93f},
                {{-70,-4.f,65}, -9.f,   0.87f},
                {{110,-4.f,60}, -14.f,  0.76f},
                {{0,-4.f,130},  -12.f,  0.68f},
                {{-85,-4.f,90}, -10.f,  1.0f},
                {{65,-4.f,120}, -15.f,  0.84f},
            };
            for(auto& tr:extraTrees){
                glDisable(GL_CULL_FACE);
                glm::mat4 tb=glm::translate(glm::mat4(1),tr.pos)
                            *glm::rotate(glm::mat4(1),glm::radians(tr.lean),glm::vec3(0,0,1))
                            *glm::scale(glm::mat4(1),glm::vec3(tr.scale));
                // Trunk
                glUseProgram(trunkSh);
                glUniformMatrix4fv(glGetUniformLocation(trunkSh,"MVP"),1,GL_FALSE,
                    glm::value_ptr(proj*view*tb));
                glUniform1f(glGetUniformLocation(trunkSh,"reveal"),rev5);
                glBindVertexArray(trunkVAO);
                glDrawElements(GL_TRIANGLES,trunkIdx,GL_UNSIGNED_INT,0);
                // Leaves
                float lr=glm::radians(-tr.lean);
                glm::vec4 tip=tb*glm::vec4(0.f,22.f,0.f,1.f);
                glm::vec3 tp=glm::vec3(tip);
                glUseProgram(leafSh);
                glUniform1f(glGetUniformLocation(leafSh,"time"),t);
                glUniform1f(glGetUniformLocation(leafSh,"reveal"),rev5);
                glBindVertexArray(leafVAO);
                for(int li=0;li<12;li++){
                    float ang=li*30.f+15.f;
                    float tilt=35.f+sinf(li*1.1f)*20.f;
                    float sc=0.8f+sinf(li*0.7f)*0.2f;
                    glm::mat4 lm=glm::translate(glm::mat4(1),tp)
                        *glm::rotate(glm::mat4(1),glm::radians(ang),glm::vec3(0,1,0))
                        *glm::rotate(glm::mat4(1),glm::radians(20.f),glm::vec3(1,0,0))
                        *glm::rotate(glm::mat4(1),glm::radians(-tilt),glm::vec3(0,0,1))
                        *glm::scale(glm::mat4(1),glm::vec3(sc*tr.scale));
                    glUniformMatrix4fv(glGetUniformLocation(leafSh,"MVP"),1,GL_FALSE,
                        glm::value_ptr(proj*view*lm));
                    glUniform1i(glGetUniformLocation(leafSh,"leafIdx"),li);
                    glDrawElements(GL_TRIANGLES,leafIdx,GL_UNSIGNED_INT,0);
                }
                glEnable(GL_CULL_FACE);
            }
        }
        // ── ROW 2+3+4: OCEAN ──────────────────────────────────────────────────
        if(rev2>0){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            // Position ocean below and in front of camera start
            glm::mat4 wm=proj*view*glm::translate(glm::mat4(1),glm::vec3(camPos.x,-4.5f,camPos.z-60.f));
            glUseProgram(watSh);
            glUniformMatrix4fv(glGetUniformLocation(watSh,"MVP"),1,GL_FALSE,glm::value_ptr(wm));
            glUniform1f(glGetUniformLocation(watSh,"time"),    t);
            glUniform1f(glGetUniformLocation(watSh,"reveal"),  rev2);
            glUniform1f(glGetUniformLocation(watSh,"waveRev"), glm::max(rev3,0.3f)); // always some wave movement
            glUniform1f(glGetUniformLocation(watSh,"foamRev"), rev4);
            glBindVertexArray(watVAO);
            glDrawElements(GL_TRIANGLES,wIdx,GL_UNSIGNED_INT,0);
            glDisable(GL_BLEND);
        }

        // ── ROW 4: SAND (rendered before water so it shows at shore) ──────
        if(rev5>0){
            glm::mat4 sm=proj*view*glm::translate(glm::mat4(1),glm::vec3(0,-4.0f,60.f));
            glUseProgram(sndSh);
            glUniformMatrix4fv(glGetUniformLocation(sndSh,"MVP"),1,GL_FALSE,glm::value_ptr(sm));
            glUniform1f(glGetUniformLocation(sndSh,"reveal"),rev5);
            glUniform1f(glGetUniformLocation(sndSh,"time"),  t);
            glBindVertexArray(sndVAO);
            glDrawElements(GL_TRIANGLES,sIdx,GL_UNSIGNED_INT,0);
        }



        // ── PUZZLE (fixed view, clears depth so always on top) ─────────────────
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
        glUseProgram(pSh);
        glUniform1i(glGetUniformLocation(pSh,"hasTex"),texID?1:0);
        if(texID){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texID);
                  glUniform1i(glGetUniformLocation(pSh,"tex"),0);}
        // When complete: shrink puzzle and move to top-left corner
        bool complete=(nxt>=(int)pieces.size());
        // Very slow transition — 8 seconds
        static float completeAnim=0.f;
        if(nxt>=(int)pieces.size()) completeAnim+=0.016f/8.f;
        float completeT=glm::clamp(completeAnim,0.f,1.f);
        float ease=completeT*completeT*(3.f-2.f*completeT);

        // Puzzle scale: 1.0 normal -> 0.22 small
        float pScale=glm::mix(1.0f,0.22f,ease);
        // Puzzle position shift: centre -> top-left
        float pShiftX=glm::mix(0.f,-0.68f,ease); // NDC shift left
        float pShiftY=glm::mix(0.f, 0.68f,ease); // NDC shift up

        // Adjusted view distance
        float pDyn=glm::mix(pDist,pDist*0.8f,ease);
        glm::mat4 puzViewDyn=glm::lookAt(glm::vec3(0,1,pDyn),glm::vec3(0,0,0),glm::vec3(0,1,0));

        // Viewport for puzzle: full screen when normal, small corner when complete
        int pvW=(int)(fw*(complete? 0.28f:1.f));
        int pvH=(int)(fh*(complete? 0.28f:1.f));
        int pvX=complete? 10 : 0;
        int pvY=complete? fh-pvH-10 : 0;
        glViewport(pvX,pvY,pvW,pvH);

        glm::mat4 pRot=glm::rotate(glm::rotate(glm::mat4(1),glm::radians(rotX),glm::vec3(1,0,0)),glm::radians(rotY),glm::vec3(0,1,0));
        for(auto& p:pieces){
            glm::mat4 model;
            if(p.placed) model=pRot*glm::translate(glm::mat4(1),p.cur);
            else{
                model=glm::translate(glm::mat4(1),p.cur);
                float tilt=p.snapT>=0?(1.f-glm::clamp(p.snapT/.6f,0.f,1.f)):1.f;
                if(tilt>0){model=glm::rotate(model,glm::radians(12.f*tilt),glm::vec3(1,0,0));
                           model=glm::rotate(model,glm::radians(-8.f*tilt),glm::vec3(0,1,0));}
            }
            glm::mat4 pPrj=glm::perspective(glm::radians(45.f),(float)pvW/pvH,.1f,100000.f);
            glm::mat4 MVP=pPrj*puzViewDyn*model;
            glUniformMatrix4fv(glGetUniformLocation(pSh,"MVP"),  1,GL_FALSE,glm::value_ptr(MVP));
            glUniformMatrix4fv(glGetUniformLocation(pSh,"model"),1,GL_FALSE,glm::value_ptr(model));
            glBindVertexArray(p.vao);
            glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0);
        }

        glViewport(0,0,fw,fh);
        glfwSwapBuffers(win);
    }
    glfwTerminate();
    return 0;
}