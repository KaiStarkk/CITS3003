#version 150

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
in ivec4 vBoneIDs;
in vec4 vBoneWeights;

out vec2 texCoord;
out vec3 N;
out vec3 pos;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 BoneTransforms[64];

void main()
{
    // Calculate bone tranformation
    mat4 boneTransform = vBoneWeights[0] * BoneTransforms[vBoneIDs[0]] +
                         vBoneWeights[1] * BoneTransforms[vBoneIDs[1]] +
                         vBoneWeights[2] * BoneTransforms[vBoneIDs[2]] +
                         vBoneWeights[3] * BoneTransforms[vBoneIDs[3]];

    // Transform position and normal with bone transform
    vec4 tPosition = boneTransform * vPosition;
    vec3 tNormal = (boneTransform * vec4(vNormal, 0.0)).xyz;

    // Transform vertex position into eye coordinates
    pos = (ModelView * tPosition).xyz;

    // Transform vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    N = normalize( (ModelView * vec4(tNormal, 0.0)).xyz );

    gl_Position = Projection * ModelView * tPosition;
    texCoord = vTexCoord;
}
