/* overall code structure for combining post-process
 * effects (with enable/disable functionality)
 * https://learnopengl.com/In-Practice/2D-Game/Postprocessing */

#version 450

uniform sampler2D screenTexture;

// why does ew::Shader not have a way to do stuff with arrays
uniform vec2 offset0;
uniform vec2 offset1;
uniform vec2 offset2;
uniform vec2 offset3;
uniform vec2 offset4;
uniform vec2 offset5;
uniform vec2 offset6;
uniform vec2 offset7;
uniform vec2 offset8;

// uuuuuuuuuugh
uniform float edgeKernel0;
uniform float edgeKernel1;
uniform float edgeKernel2;
uniform float edgeKernel3;
uniform float edgeKernel4;
uniform float edgeKernel5;
uniform float edgeKernel6;
uniform float edgeKernel7;
uniform float edgeKernel8;

uniform int invert;
uniform int edgeDetection;

in vec2 TexCoords;

out vec4 FragColor;

void main() {
    vec4 color = texture(screenTexture, TexCoords);
    vec3 sampling[9];

    // build an array for the edge kernel manually
    float edge_kernel[9] = {edgeKernel0, edgeKernel1, edgeKernel2, edgeKernel3,
        edgeKernel4, edgeKernel5, edgeKernel6, edgeKernel7, edgeKernel8};

    // sample from texture offsets if using convolution matrix
    if(edgeDetection > 0) { // would check multiple bools if there were multiple kernel effects
        sampling[0] = vec3(texture(screenTexture, TexCoords.st + offset0));
        sampling[1] = vec3(texture(screenTexture, TexCoords.st + offset1));
        sampling[2] = vec3(texture(screenTexture, TexCoords.st + offset2));
        sampling[3] = vec3(texture(screenTexture, TexCoords.st + offset3));
        sampling[4] = vec3(texture(screenTexture, TexCoords.st + offset4));
        sampling[5] = vec3(texture(screenTexture, TexCoords.st + offset5));
        sampling[6] = vec3(texture(screenTexture, TexCoords.st + offset6));
        sampling[7] = vec3(texture(screenTexture, TexCoords.st + offset7));
        sampling[8] = vec3(texture(screenTexture, TexCoords.st + offset8));
    }

    /* edge detection and invert effects sourced from
     * https://learnopengl.com/Advanced-OpenGL/Framebuffers */
    if(edgeDetection > 0) {
        for(int i = 0; i < 9; i++)
            color += vec4(sampling[i] * edge_kernel[i], 0.0f);
        color.a = 1.0f;
    }
    if(invert > 0) {
        color = vec4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1.0);
    }

    FragColor = color;
}