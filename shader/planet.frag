#version 330 core
out vec4 FragColor;
in vec3 FragPos; in vec3 Normal; in vec2 TexCoords;
uniform sampler2D texture1; uniform vec3 lightPos; uniform vec3 viewPos; uniform bool isSun;
void main(){
    vec3 color = texture(texture1, TexCoords).rgb;
    if(isSun){ FragColor = vec4(color*2.0,1.0); return; }
    float ambientStrength=0.15;
    vec3 ambient = ambientStrength*color;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir),0.0);
    vec3 diffuse = diff*color;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir),0.0),32.0);
    vec3 specular = vec3(0.4)*spec;
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.002 * distance + 0.000001 * distance * distance);
    vec3 result = (ambient + attenuation*(diffuse + specular));
    FragColor = vec4(result,1.0);
}
