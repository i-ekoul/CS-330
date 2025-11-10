#version 330 core
out vec4 fragmentColor;

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
}; 

struct DirectionalLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct PointLight {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       

    bool bActive;
};

#define TOTAL_POINT_LIGHTS 5

uniform bool bUseTexture=false;
uniform bool bUseLighting=false;
uniform vec4 objectColor = vec4(1.0f);
uniform vec3 viewPosition;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[TOTAL_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform sampler2D objectTexture;
uniform vec2 UVscale = vec2(1.0f, 1.0f);

// -------------------------
// added!: emissive flame controls (no vertex edits required)
// -------------------------
uniform bool  bEmissive = false;     // when true, skip Phong and output emissive color
uniform bool  bFlame    = false;     // this fragment belongs to a flame
uniform float flameTime = 0.0;       // seconds (animate flicker)
uniform float flameBaseY = 0.0;      // world-space Y at the base of this flame
uniform float flameHeight = 1.0;     // world-space height span of this flame

// added!: tiny hash to dither alpha a bit (reduces banding)
float hash11(float p) { return fract(sin(p) * 43758.5453123); }

// the scaled texture coordinate to use in calculations
vec2 fragmentTextureCoordinateScaled = fragmentTextureCoordinate * UVscale;

// function prototypes
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // -----------------------------------------------------------------------
    // added!: emissive flame branch (early return; ignores Phong lighting)
    //         Color ramp by height t in [0,1] and subtle time flicker.
    // -----------------------------------------------------------------------
    if (bEmissive && bFlame) // added!:
    {
        // normalized vertical coordinate of this fragment within the flame
        float t = clamp((fragmentPosition.y - flameBaseY) / max(flameHeight, 1e-4), 0.0, 1.0); // added!:

        // ENHANCED REALISTIC FLAME PALETTE - More natural fire colors
        vec3 c0 = vec3(0.15, 0.18, 0.35);  // Deep blue base
        vec3 c1 = vec3(1.00, 1.00, 0.95);   // Brilliant white core
        vec3 c2 = vec3(1.00, 0.85, 0.25);   // Bright yellow-orange
        vec3 c3 = vec3(1.00, 0.55, 0.15);   // Orange
        vec3 c4 = vec3(0.95, 0.25, 0.05);   // Deep red
        vec3 c5 = vec3(0.60, 0.10, 0.02);   // Dark red tips

        vec3 col;
        if (t < 0.10) {
            float u = smoothstep(0.00, 0.10, t);
            col = mix(c0, c1, u);
        } else if (t < 0.25) {
            float u = smoothstep(0.10, 0.25, t);
            col = mix(c1, c2, u);
        } else if (t < 0.45) {
            float u = smoothstep(0.25, 0.45, t);
            col = mix(c2, c3, u);
        } else if (t < 0.70) {
            float u = smoothstep(0.45, 0.70, t);
            col = mix(c3, c4, u);
        } else {
            float u = smoothstep(0.70, 1.00, t);
            col = mix(c4, c5, u);
        }

        // ENHANCED REALISTIC FLICKER - Multi-frequency turbulence
        float f1 = 0.5 + 0.5 * sin(flameTime * 8.5 + fragmentPosition.x * 3.17);
        float f2 = 0.5 + 0.5 * sin(flameTime * 5.2 + fragmentPosition.z * 5.41);
        float f3 = 0.5 + 0.5 * sin(flameTime * 12.1 + fragmentPosition.y * 2.73);
        float f4 = 0.5 + 0.5 * sin(flameTime * 3.7 + (fragmentPosition.x + fragmentPosition.z) * 1.89);
        
        float flick = 0.85 + 0.30 * (0.3 * f1 + 0.3 * f2 + 0.2 * f3 + 0.2 * f4);
        col *= flick;
        
        // Add heat shimmer effect
        float shimmer = 1.0 + 0.1 * sin(flameTime * 15.0 + fragmentPosition.y * 8.0);
        col *= shimmer;

        // added!: alpha stronger near base, fades to tip; tiny dithering
        float a = mix(0.95, 0.18, smoothstep(0.05, 1.00, t));
        a *= 0.95 + 0.05 * hash11(gl_FragCoord.x + gl_FragCoord.y);

        fragmentColor = vec4(col, clamp(a, 0.0, 1.0)); // added!:
        return; // added!:
    }

    // ----- existing pipeline below (unchanged) -----
    if(bUseLighting == true)
    {
        vec3 phongResult = vec3(0.0f);
        // properties
        vec3 norm = normalize(fragmentVertexNormal);
        vec3 viewDir = normalize(viewPosition - fragmentPosition);
    
        // phase 1: directional lighting
        if(directionalLight.bActive == true)
        {
            phongResult += CalcDirectionalLight(directionalLight, norm, viewDir);
        }
        // phase 2: point lights
        for(int i = 0; i < TOTAL_POINT_LIGHTS; i++)
        {
	        if(pointLights[i].bActive == true)
            {
                phongResult += CalcPointLight(pointLights[i], norm, fragmentPosition, viewDir);   
            }
        } 
        // phase 3: spot light
        if(spotLight.bActive == true)
        {
            phongResult += CalcSpotLight(spotLight, norm, fragmentPosition, viewDir);    
        }
    
        if(bUseTexture == true)
        {
            fragmentColor = vec4(phongResult, (texture(objectTexture, fragmentTextureCoordinateScaled)).a);
        }
        else
        {
            fragmentColor = vec4(phongResult, objectColor.a);
        }
    }
    else
    {
        if(bUseTexture == true)
        {
            fragmentColor = texture(objectTexture, fragmentTextureCoordinateScaled);
        }
        else
        {
            fragmentColor = objectColor;
        }
    }
}

// calculates the color when using a directional light.
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 lightDirection = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDirection), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    if(bUseTexture == true)
    {
        ambient = light.ambient * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
        specular = light.specular * spec * material.specularColor * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
    }
    else
    {
        ambient = light.ambient * vec3(objectColor);
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(objectColor);
        specular = light.specular * spec * material.specularColor * vec3(objectColor);
    }
    
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular= vec3(0.0f);

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
   
    if(bUseTexture == true)
    {
        ambient = light.ambient * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
        specular = light.specular * specularComponent * material.specularColor;
    }
    else
    {
        ambient = light.ambient * vec3(objectColor);
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(objectColor);
        specular = light.specular * specularComponent * material.specularColor;
    }
    
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    if(bUseTexture == true)
    {
        ambient = light.ambient * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
        specular = light.specular * spec * material.specularColor * vec3(texture(objectTexture, fragmentTextureCoordinateScaled));
    }
    else
    {
        ambient = light.ambient * vec3(objectColor);
        diffuse = light.diffuse * diff * material.diffuseColor * vec3(objectColor);
        specular = light.specular * spec * material.specularColor * vec3(objectColor);
    }
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}