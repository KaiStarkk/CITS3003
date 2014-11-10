#version 150

in vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
in vec3 pos;
in vec3 N;

out vec4 fColor;

uniform sampler2D texture;
uniform float texScale;

uniform vec4 LightPosition1;
uniform vec3 AmbientProduct1, DiffuseProduct1, SpecularProduct1;
uniform vec4 LightPosition2;
uniform vec3 AmbientProduct2, DiffuseProduct2, SpecularProduct2;
uniform float Shininess;
uniform float Alpha;

uniform mat4 View;

void
main()
{
    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    vec3 E = normalize(-pos);   // Direction to the eye/camera

    //---------
    // Light 1
    //---------

    // The vector to the light from the vertex
    vec3 Lvec1 = LightPosition1.xyz - pos;

    float Ldist1 = length(Lvec1); // Distance from light source
    float Lscale1 = 1.0 / (Ldist1 * Ldist1); // Light scaling factor

    vec3 L1 = normalize(Lvec1);   // Direction to the light source
    vec3 H1 = normalize(L1 + E);  // Halfway vector

    // Compute terms in the illumination equation
    vec3 ambient1 = AmbientProduct1;

    float Kd1 = max(dot(L1, N), 0.0);
    vec3  diffuse1 = Kd1 * DiffuseProduct1 * Lscale1;

    float Ks1 = pow(max(dot(N, H1), 0.0), Shininess);
    float Si1 = dot(SpecularProduct1, vec3(0.33, 0.33, 0.33));
    vec3  specular1 = Ks1 * vec3(Si1, Si1, Si1) * Lscale1;
    
    if( dot(L1, N) < 0.0 ) {
      specular1 = vec3(0.0, 0.0, 0.0);
    }

    //---------
    // Light 2
    //---------

    // The vector to the light from the origin
    vec3 Lvec2 = (LightPosition2 - (View * vec4(0.0, 0.0, 0.0, 1.0))).xyz;

    // float Ldist2 = length(Lvec2); // Distance from light source
    // float Lscale2 = 1.0 / (Ldist2 * Ldist2); // Light scaling factor

    vec3 L2 = normalize(Lvec2);   // Direction to the light source
    vec3 H2 = normalize(L2);  // Halfway vector

    // Compute terms in the illumination equation
    vec3 ambient2 = AmbientProduct2;

    float Kd2 = max(dot(L2, N), 0.0);
    vec3  diffuse2 = Kd2 * DiffuseProduct2; // * Lscale2;

    float Ks2 = pow(max(dot(N, H2), 0.0), Shininess);
    float Si2 = dot(SpecularProduct2, vec3(0.33, 0.33, 0.33));
    vec3  specular2 = Ks2 * vec3(Si2, Si2, Si2); // * Lscale2;
    
    if( dot(L2, N) < 0.0 ) {
      specular2 = vec3(0.0, 0.0, 0.0);
    }

    vec4 color;

    color.rgb = globalAmbient + ambient1 + diffuse1 + ambient2 + diffuse2;
    color.a = 1.0;

    fColor = color * texture2D( texture, texCoord * texScale );
    fColor.rgb = fColor.rgb + specular1;
    fColor.a = Alpha;
}
