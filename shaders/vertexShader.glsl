#version 330 core
layout (location = 0) in vec3 inVertexPosition;
layout (location = 1) in vec3 inVertexNormal;
layout (location = 2) in vec2 inTextureCoordinate;

out vec3 fragmentPosition;
out vec3 fragmentVertexNormal;
out vec2 fragmentTextureCoordinate;

// added!:
out float vFlameT;   // normalized vertical coordinate 0..1 along the flame

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// added!: flame controls
uniform bool  bFlame;            // this is a flame pass
uniform float flameTime;         // seconds
uniform float flameBaseY;        // world-space base Y of this flame
uniform float flameHeight;       // its current height (matches model.y scale you set)
uniform float flameWobbleAmp;    // ~0.06–0.10 (world units)
uniform float flameTwist;        // radians of twist over full height (e.g., 0.6)

void main()
{
    // base world-space position (no deformation yet)
    vec3 worldPos = vec3(model * vec4(inVertexPosition, 1.0));

    // default passthrough
    fragmentVertexNormal      = inVertexNormal;      // keep your pipeline as-is
    fragmentTextureCoordinate = inTextureCoordinate;

    if (bFlame) {
        // ------------------ flame deformation path ------------------  // added!:
        vec3 wp = worldPos;

        // normalized vertical param 0..1 inside this flame
        float t = clamp((wp.y - flameBaseY) / max(flameHeight, 1e-4), 0.0, 1.0);
        vFlameT = t;

        // sideways wobble (grows with height)
        float wPhase = flameTime * 6.0 + (wp.x * 3.17 + wp.z * 5.41);
        float wobble = flameWobbleAmp * (0.30 + 0.70 * t);
        wp.x += wobble * sin(wPhase + t * 10.0);
        wp.z += wobble * cos(wPhase * 0.8 + t * 12.0);

        // gentle twist around Y that increases with height
        float ang = flameTwist * t;
        float cs = cos(ang), sn = sin(ang);
        vec2 xz = mat2(cs, -sn, sn, cs) * vec2(wp.x, wp.z);
        wp.x = xz.x; wp.z = xz.y;

        // write results
        fragmentPosition = wp;                                     // added!:
        gl_Position      = projection * view * vec4(wp, 1.0);      // added!:
    } else {
        // ------------------ normal (non-flame) path ------------------
        vFlameT          = 0.0;                                    // added!:
        fragmentPosition = worldPos;
        gl_Position      = projection * view * vec4(worldPos, 1.0);
    }
}
