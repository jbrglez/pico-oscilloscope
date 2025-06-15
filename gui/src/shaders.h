

char graph_plot_vs[] = "\
#version 330                                                \n\
                                                            \n\
layout(location = 0) in float vertexPosition_x;             \n\
layout(location = 1) in float vertexPosition_y;             \n\
                                                            \n\
uniform mat4 mvp;                                           \n\
uniform float currentTime;                                  \n\
                                                            \n\
void main()                                                 \n\
{                                                           \n\
    vec2 pos = vec2(vertexPosition_x, vertexPosition_y);    \n\
    gl_Position = mvp * vec4(pos, 0.0, 1.0);                \n\
    gl_PointSize = 10;                                      \n\
}                                                           \n\
";


char graph_plot_line_fs[] = "\
#version 330                \n\
                            \n\
uniform vec4 plotColor;     \n\
                            \n\
out vec4 finalColor;        \n\
                            \n\
void main()                 \n\
{                           \n\
    finalColor = plotColor; \n\
}                           \n\
";


char graph_plot_point_fs[] = "\
#version 330                                            \n\
                                                        \n\
uniform vec4 plotColor;                                 \n\
                                                        \n\
float my_circle(in vec2 _p, in float _r) {              \n\
    float l_sq = dot(_p, _p);                           \n\
    float r_sq = _r * _r;                               \n\
    return step(0, r_sq - l_sq);                        \n\
}                                                       \n\
                                                        \n\
void main()                                             \n\
{                                                       \n\
    gl_FragColor = vec4(plotColor.rgb, plotColor.a * my_circle(gl_PointCoord.xy - vec2(0.5), 0.5));\n\
}                                                       \n\
";


char graph_window_fs[] = "\
#version 330                                            \n\
                                                        \n\
uniform vec4 color;                                     \n\
uniform sampler2D texture1;                             \n\
in vec2 tex_coord;                                      \n\
                                                        \n\
out vec4 finalColor;                                    \n\
                                                        \n\
void main()                                             \n\
{                                                       \n\
    finalColor = texture(texture1, tex_coord);          \n\
}                                                       \n\
";


char graph_window_vs[] = "\
#version 330                                                    \n\
                                                                \n\
layout(location = 0) in vec3 vertexPosition;                    \n\
layout(location = 1) in vec2 texIn;                             \n\
                                                                \n\
uniform mat4 mvp;                                               \n\
uniform float currentTime;                                      \n\
                                                                \n\
out vec2 tex_coord;                                             \n\
                                                                \n\
void main()                                                     \n\
{                                                               \n\
    gl_Position = mvp * vec4(vertexPosition.xy, 0.0, 1.0);      \n\
    tex_coord = texIn;                                          \n\
}                                                               \n\
";
