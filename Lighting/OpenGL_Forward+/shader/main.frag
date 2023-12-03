#version 430 core

// ----------------------------------------------------------------------------
//
// Makros
//
// ----------------------------------------------------------------------------

#define GLOBAL_AMBIENT_FACTOR (0.25)

// ----------------------------------------------------------------------------
//
// Typen
//
// ----------------------------------------------------------------------------

/**
 * @brief Materialparameter
 *
 * Mit diesem struct können die Materialparameter einer Oberfläche gespeichert werden. Im Host-Code
 * gibt es ebenfalls ein struct mit dem Bezeichner Material. Die Speicherrepräsentation der structs
 * ist _nicht_ aufeinander abgestimmt.
 */
struct Material {
    vec4 color;                 /**< Beleuchtungsunabhängige Farbe */
    float ambientReflection;    /**< Ambienter Reflexionskoeffizient */
    float diffuseReflection;    /**< Diffuser Reflexionskoeffizient */
    float specularReflection;   /**< Spekularer Reflexionskoeffizient */
    float shininess;            /**< Wahrgenommene Glattheit der Oberfläche */
    bool hasTexture;            /**< Gibt an, ob das Material Reflexionskoeffizienten aus einer
                                     Textur bezieht */
};

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

in vec3 fWorldNormal;                       /**< Normale des Vertexes im Welt-Koordinatensystem */
in vec4 fWorldPosition;                     /**< Position des Vertex im Welt-Koordinatensystem */
in vec2 fTexCoord;                          /**< Textur-Koordinate */

out vec4 FragColor;                         /**< Farbe des Fragments */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

/** Textur mit Reflexionskoeffizienten */
layout (binding = 0) uniform sampler2D ObjectMaterialTexture;

uniform mat4 ViewMatrix;                    /**< Transformation vom Welt- ins Kamera-Koordinatensystem*/

uniform Material ObjectMaterial;            /**< Material das gerendert werden soll */

uniform int NumLights;                      /**< Anzahl der Lichtquellen */
uniform ivec2 TileSize;                     /**< Größe der Kacheln (Breite und Höhe) */
uniform ivec2 NumTiles;                     /**< Anzahl der Kacheln auf der X- und Y-Achse */
uniform bool RenderHeatmap;                 /**< Gibt an, ob aktuell die Heatmap gerendert wird */

/**
 * @brief Punktlichtquelle mit isotroper Abstrahlung
 *
 * Parameter einer Punkt-Lichtquelle mit isotroper Abstrahlung. Die Punktlichtquellen werden über ein Buffer Object
 * an den Shader übergeben. Deswegen gibt es dieses struct auch im Host-Code. Die Speicherrepräsentation der structs
 * ist aufeinander abgestimmt und muss bei einer Änderung berücksichtigt werden.
 */
struct PointLight {
    vec3 position;              /**< Position der Lichtquelle */
    float radius;               /**< Radius der Lichtquelle */
    vec4 color;                 /**< Anteil der Lichtquelle an der diffusen Beleuchtung */
};



/**
 * @brief Buffer für die Lichtquellen
 *
 * In diesem Buffer werden die Informationen zu den Lichtquellen gespeichert. Im Host
 * gibt es eine C-Repräsentation des structs PointLight, die an das Speicherlayout für
 * Uniform (und auch Shader Storage Buffer) angepasst ist.
 *
 * @remarks Dieser Buffer wird auch vom Shaderprogramm "Tile" verwendet. Es muss darauf
 *          geachtet werden, dass der binding-Punkt übereinstimmt.
 */
layout (std430, binding = 0) buffer LightsBuffer {
    PointLight lights[];
};

/**
 * @brief Buffer für die Indizes der sichtbaren Lichtquellen
 *
 * In diesem Buffer wird für jede einzelne Kachel gespeichert, welche Lichtquellen den
 * Pyramidenstumpf der Kachel überlappen.
 *
 * Layout:
 *
 * Kachel 1                  Kachel 2
 * |                         |
 * v                         v
 * +---+---+---+---+-----+---+-------
 * | N | 0 | 1 | 2 | ... | K | ...
 * +---+---+---+---+-----+---+-------
 *
 * Für jede Kachel wird zuerst die Anzahl der Lichtquellen (N) und anschließend ihre
 * Indizes (0..) gespeichert. Die Kapazität für jede Kachel ist gleich der Anzahl der
 * Lichtquellen insgesamt. So muss nicht mit einem Atomic Counter gearbeitet
 * werden, um für jede Kachel einen unterschiedlich größen Speicherbereich zu
 * reservieren. Außerdem entfällt damit die Notwendigkeit für Pointer (Indizes[0]),
 * um den Anfang der Indices einer Kachel im Buffer zu speichern. Somit ist auch
 * keine weitere Textur (oder weiterer Buffer) notwendig, um diese Pointer
 * (Indices) für jede Kachel zu speichern.
 *
 * [0] Da GLSL keine Pointertypen hat, werden Pointerstrukturen typischerweise
 *     mit Indices auf Array-Elemente (in Buffern) implementiert.
 */
layout (std430, binding = 1) buffer VisibleLightIndicesBuffer {
    int visibleLightIndices[];
};


// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------
/**
 * @brief W-Division
 *
 * Führt eine W-Division für den übergebenen 4D-Vektor aus.
 *
 * @param v       Für diesen Vektor wird die W-Division ausgeführt
 *
 * @return Resultat der W-Division
 */
vec3 wdiv(vec4 v) {
    return v.xyz / v.w;
}
/**
 * @brief Berechnet die Kameraposition im Weltkoordinatensystem
 *
 * Berechnet die Kameraposition, indem der Nullpunkt des Betrachterkoordinatensystems
 * mit der Inversen View Matrix in das Welt-Koordinatensystem transformiert wird.
 *
 * @param viewMatrix    ViewMatrix
 *
 * @return Position der Kamera im Weltkoordinatensystem
 */
vec3 makeCameraWorldPosition(mat4 viewMatrix) {
    return wdiv(inverse(viewMatrix)*vec4(0, 0, 0, 1));
}

/**
 * @brief Oberflächenpunkt mit einer Punktlichtquelle beleuchten
 *
 * Mit dieser Funktion kann die Beleuchtung für einen Punkt auf einer Oberfläche
 * berechnet werden. Dazu wird ein vereinfachtes Phong-Modell verwendet.
 *
 * @param material              Materialeigenschaften des Oberflächenpunktes
 * @param pointLight            Eigenschaften der Punktlichtquelle
 * @param baseColor             Farbe des Oberflächenpunktes (vereinfachtes Phong)
 * @param cameraWorldPosition   Position der Kamera im Weltkoordinatensystem
 * @param worldPosition         Position des Oberflächenpunktes im Weltkoordinatensystem
 * @param worldNormal           Normale des Oberflächenpunktes im Weltkoordinatensystem
 *
 * @return Beleuchtung für den Oberflächenpunkt
 */
vec4 shadePointLight(Material material,
                     PointLight pointLight,
                     vec4 baseColor,
                     vec3 cameraWorldPosition,
                     vec3 worldPosition,
                     vec3 worldNormal)
{
    vec3 cameraDirection = normalize(cameraWorldPosition - worldPosition);
    vec3 lightDirection = normalize(pointLight.position - worldPosition);
    vec3 reflectionDirection = reflect(-lightDirection, worldNormal);

    vec4 i_amb  = baseColor*material.ambientReflection*pointLight.color;
    vec4 i_diff = max(0, dot(worldNormal, lightDirection))*baseColor*material.diffuseReflection*pointLight.color;
    vec4 i_spec = pow(max(0, dot(reflectionDirection, cameraDirection)), material.shininess)*material.specularReflection*pointLight.color;

    float distance = length(pointLight.position - worldPosition);
    // Hier ein sehr einfaches und unrealistischen Fall-off Modell.
    float d = max(0, 1 - (1.0/pointLight.radius)*distance);

    return d*(i_amb + i_diff + i_spec);
}

/**
 * @brief Berechnung der Farbe für die Heatmap
 *
 * Mit dieser Funktion kann die Farbe der Heatmap für einen Wert berechnet werden.
 *
 * @param value     Wert zwischen 0 und 1 für die Berechnung der Heatmap
 *
 * @return Farbe der Heatmap für den übergebenen Wert
 */
vec4 heatmap(float value) {
    vec3 color = vec3(0);

    // Farbewerte der Heatmap
    const vec3 color0 = vec3(0, 0, 1);
    const vec3 color1 = vec3(0, 1, 0);
    const vec3 color2 = vec3(1, 1, 0);
    const vec3 color3 = vec3(1, 0, 0);
    const vec3 color4 = vec3(1, 0, 1);

    if (value >= 0 && value < 0.25) {
        color = mix(color0, color1, value*4);
    } else if (value >= 0.25 && value < 0.5) {
        color = mix(color1, color2, (value - 0.25)*4);
    } else if (value >= 0.5  && value < 0.75) {
        color = mix(color2, color3, (value - 0.5)*4);
    } else if (value >= 0.75 && value < 1.0) {
        color = mix(color3, color4, (value - 0.75)*4);
    }
    return vec4(color, 1);
}




/**
 * Einsprungpunkt für den Fragment-Shader
 */
void main() {
    // Invertierung der Texturkoordinaten auf der Y-Achse, da die meisten OBJ-Dateien
    // Texturkoordinaten enthalten, bei denen der Koordinatenursprung oben links ist.
    vec2 invTexCoord = vec2(fTexCoord.x, 1 - fTexCoord.y);

    vec4 materialColor = ObjectMaterial.hasTexture ? texture(ObjectMaterialTexture, invTexCoord) : ObjectMaterial.color;

    // Position der Lichtquellenindizes in Buffer Object berechnen.
    ivec2 tilePosition = ivec2(gl_FragCoord.xy) / TileSize.xy;
    uint linearWorkGroupIndex = uint(NumTiles.x*tilePosition.y + tilePosition.x);

    // Anzahl der Lichtquellen wird vor den Indizes selbst gespeichert.
    int numLights = visibleLightIndices[(NumLights + 1)*linearWorkGroupIndex];

    // Beitrag jeder einzelnen Lichtquelle zur Beleuchtung des Oberflächenpunktes
    // berechnen und die Intensitäten aufaddieren-
    vec4 color = vec4(0, 0, 0, 1);    
    for (int lightIndex = 0; lightIndex < numLights; ++lightIndex) {
        int index = visibleLightIndices[(NumLights + 1)*linearWorkGroupIndex + lightIndex + 1];
        PointLight light = lights[index];

        vec4 illumination = shadePointLight(ObjectMaterial,
                                            light,
                                            materialColor,
                                            makeCameraWorldPosition(ViewMatrix),
                                            wdiv(fWorldPosition),
                                            fWorldNormal);

        color += illumination;
    }

    // Hier wird eine ambient Beleuchtung addiert, da die Verteilung der Lichtquellen
    // typischerweise so ungleichmäßig ist, dass nicht jeder Teil der Szene beleuchtet
    // wird. Durch das Addieren der Materialfarbe wird eine ambiente Beleuchtung
    // mit komplett weißem Licht erreicht.
    FragColor = color + GLOBAL_AMBIENT_FACTOR*materialColor;

    // Heatmap leicht transparent über die Szene legen.
    if (RenderHeatmap) {
      FragColor = mix(FragColor, heatmap(float(numLights)/NumLights), 0.7);
    }
}
