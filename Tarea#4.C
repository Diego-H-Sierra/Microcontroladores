#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NOMBRE 50
#define ARCHIVO "alumnos.dat"

// Definición de la estructura alumno
typedef struct {
    char nombre[MAX_NOMBRE];
    int edad;
    float promedio;
} Alumno;

// Prototipos de funciones
void mostrar_menu();
void leer_alumno(Alumno *alumno);
void grabar_en_disco(Alumno *alumno);
void cargar_desde_disco();
void liberar_memoria(Alumno *alumnos[], int num_alumnos);

int main() {
    int opcion;
    Alumno *alumnos[100]; // Array de punteros a estructuras Alumno
    int num_alumnos = 0;

    do {
        mostrar_menu();
        scanf("%d", &opcion);

        switch (opcion) {
            case 1:
                printf("Ingrese los datos del alumno:\n");
                alumnos[num_alumnos] = (Alumno *)malloc(sizeof(Alumno));
                leer_alumno(alumnos[num_alumnos]);
                grabar_en_disco(alumnos[num_alumnos]);
                num_alumnos++;
                break;
            case 2:
                cargar_desde_disco();
                break;
            case 3:
                printf("Saliendo del programa.\n");
                liberar_memoria(alumnos, num_alumnos);
                break;
            default:
                printf("Opción no válida. Intente nuevamente.\n");
                break;
        }
    } while (opcion != 3);

    return 0;
}

void mostrar_menu() {
    printf("\nMenú:\n");
    printf("1. Ingresar info de alumno\n");
    printf("2. Leer info de alumno\n");
    printf("3. Salir\n");
    printf("Seleccione una opción: ");
}

void leer_alumno(Alumno *alumno) {
    printf("Nombre: ");
    scanf(" %[^\n]s", alumno->nombre); // Lee el nombre con espacios
    printf("Edad: ");
    scanf("%d", &alumno->edad);
    printf("Promedio: ");
    scanf("%f", &alumno->promedio);
}

void grabar_en_disco(Alumno *alumno) {
    FILE *archivo;
    archivo = fopen(ARCHIVO, "ab"); // Abre el archivo en modo agregar binario

    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return;
    }

    fwrite(alumno, sizeof(Alumno), 1, archivo); // Escribe la estructura completa en el archivo
    fclose(archivo);

    printf("Alumno guardado en disco correctamente.\n");
}

void cargar_desde_disco() {
    FILE *archivo;
    Alumno alumno;
    int num_alumnos_leidos = 0;

    archivo = fopen(ARCHIVO, "rb"); // Abre el archivo en modo leer binario

    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return;
    }

    printf("\nContenido de alumnos en disco:\n");

    while (fread(&alumno, sizeof(Alumno), 1, archivo) == 1) {
        printf("Nombre: %s, Edad: %d, Promedio: %.2f\n", alumno.nombre, alumno.edad, alumno.promedio);
        num_alumnos_leidos++;
    }

    if (num_alumnos_leidos == 0) {
        printf("No hay alumnos guardados en el archivo.\n");
    }

    fclose(archivo);
}

void liberar_memoria(Alumno *alumnos[], int num_alumnos) {
    for (int i = 0; i < num_alumnos; i++) {
        free(alumnos[i]); // Libera la memoria de cada estructura Alumno
    }
}
