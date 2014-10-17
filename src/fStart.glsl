#version 150

in vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
in vec3 pos;
in vec3 N;

out vec4 fColor;

uniform sampler2D texture;
uniform vec4 LightPosition;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

void
main()
{
    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    // The vector to the light from the vertex    
    vec3 Lvec = LightPosition.xyz - pos;

    float Ldist = length(Lvec); // Distance from light source
    float Lscale = 1.0 / (Ldist * Ldist); // Light scaling factor

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector

    float Kd = max( dot(L, N), 0.0 );
    vec3  diffuse = Kd * DiffuseProduct * Lscale;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    vec3  specular = Ks * SpecularProduct * Lscale;
    
    if( dot(L, N) < 0.0 ) {
      specular = vec3(0.0, 0.0, 0.0);
    }

    vec4 color;

    color.rgb = globalAmbient + ambient + diffuse + specular;
    color.a = 1.0;

    fColor = color * texture2D( texture, texCoord * 2.0 );
}
