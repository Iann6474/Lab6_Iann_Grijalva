/* Universidad del Valle de Guatemala
 * IE2023 - Programación de Microcontroladores
 * Created: 21/04/2025
 * Author :Iann Grijalva-23055
 * Descripción: Este codigo permite ejecutar el Lab #6
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#define BAUD 9600
#define UBRR_VALUE ((F_CPU/(16UL*BAUD))-1)
#define MAX_BUFFER 64  // Tamaño máximo del buffer para recibir cadenas

volatile char Buffer[MAX_BUFFER];
volatile uint8_t Indice = 0;
volatile uint8_t Ccompletada = 0;
volatile uint8_t ModoActual = 0; // 0: Menú, 1: Enviar datos por USART (ASCII), 2: Leer potenciómetro
volatile uint16_t ValorPotenciometro = 0;
volatile uint8_t LeerPotentiometro = 0;

void inicializar_UART(void);
void enviar_CaracterUART(unsigned char dato);
void enviar_CadenaUART(const char* cadena);
void mostrar_CaracterEnLEDs(unsigned char caracter);
void inicializar_ADC(void);
uint16_t leer_ADC(uint8_t canal);
void mostrarMenu(void);

int main(void) {
    // Configurar pines para LEDs
    DDRD |= 0b11111000;  // PIND3-PIND7 como salidas (5 bits)
    DDRB |= 0b00000111;  // PINB0-PINB2 como salidas (3 bits)
    
    // Inicializar UART con interrupciones
    inicializar_UART();
    
    // Inicializar ADC
    inicializar_ADC();
    
    // Habilitar interrupciones globales
    sei();
    
    // Mostrar el mensaje de bienvenida y el menú
    enviar_CadenaUART("\r\n--- Bienvenido al LAB #6 Iann Grijalva ---\r\n");
    mostrarMenu();
    
    char valorStr[10]; // Buffer para convertir valor a cadena
    
    while (1) {
        // Estado del programa basado en el modo seleccionado
        switch (ModoActual) {
            case 0: // Modo Menú - Esperar entrada del usuario
                // No hacer nada, esperar a que llegue un comando por ISR
                break;
                
            case 1: // Modo Enviar datos por USART (ASCII)
                // La ISR se encarga de procesar los caracteres y mostrarlos en los LEDs
                break;
                
            case 2: // Modo Lectura de Potenciómetro
                if (LeerPotentiometro) {
                    // Leer el valor del potenciómetro y enviarlo por USART
                    ValorPotenciometro = leer_ADC(6); // A6
                    
                    // Convertir el valor a string
                    sprintf(valorStr, "%d", ValorPotenciometro);
                    
                    // Enviar el valor
                    enviar_CadenaUART("Valor del Potenciometro: ");
                    enviar_CadenaUART(valorStr);
                    enviar_CadenaUART("\r\n");
                    mostrar_CaracterEnLEDs(ValorPotenciometro);
                    
                    // Esperar un poco antes de la siguiente lectura
                    _delay_ms(500);
                }
                break;
        }
        
        // Si se completa una cadena y estamos en modo 1 o 2, verificar si es  'l' o 'L' para volver al menú
        if (Ccompletada) {
            if ((ModoActual == 1 || ModoActual == 2) && 
                (Buffer[0] == 'M' || Buffer[0] == 'm' || Buffer[0] == 'L' || Buffer[0] == 'l') && Buffer[1] == '\0') {
                // Volver al menú
                ModoActual = 0;
                LeerPotentiometro = 0; // Detener lectura del potenciómetro
                mostrarMenu();
            }
            
            // Resetear para la próxima cadena
            Indice = 0;
            Ccompletada = 0;
        }
    }
}
    
// Inicializar UART con interrupciones
void inicializar_UART(void) {
    // Configurar velocidad de bauds
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;
    
    // Habilitar transmisión y recepción, y la interrupción de recepción completa
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
    
    // Formato de trama: 8 bits de datos, 1 bit de parada, sin paridad
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Enviar un carácter por UART
void enviar_CaracterUART(unsigned char dato) {
    // Esperar a que el buffer de transmisión esté vacío
    while (!(UCSR0A & (1 << UDRE0)));
    
    // Poner el dato en el buffer de transmisión
    UDR0 = dato;
}

// Enviar una cadena por UART
void enviar_CadenaUART(const char* cadena) {
    // Enviar cada carácter hasta encontrar el terminador nulo
    while (*cadena != '\0') {
        enviar_CaracterUART(*cadena);
        cadena++;
    }
}

// Mostrar un carácter en los LEDs distribuidos
void mostrar_CaracterEnLEDs(unsigned char caracter) {
    // Extraer los 5 bits menos significativos para PIND3-PIND7
    PORTD = (PORTD & 0b00000111) | ((caracter & 0b00011111) << 3);
    
    // Extraer los 3 bits más significativos para PINB0-PINB2
    PORTB = (PORTB & 0xF8) | ((caracter >> 5) & 0b00000111);
}

// Inicializar el ADC
void inicializar_ADC(void) {
    // Configurar el ADC: referencia AVCC, ajuste a la derecha
    ADMUX = (1 << REFS0);
    
    // Habilitar ADC, prescaler de 128 (125kHz con clock de 16MHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Leer el valor del ADC en un canal específico
uint16_t leer_ADC(uint8_t canal) {
    // Seleccionar el canal (solo los 4 bits inferiores)
    ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
    
    // Iniciar la conversión
    ADCSRA |= (1 << ADSC);
    
    // Esperar a que la conversión termine
    while (ADCSRA & (1 << ADSC));
    
    // Retornar el resultado
    return ADC;
}

// Mostrar el menú de opciones
void mostrarMenu(void) {
    enviar_CadenaUART("\r\n------ MENU PRINCIPAL ------\r\n");
    enviar_CadenaUART("1. Enviar ASCII\r\n");
    enviar_CadenaUART("2. Leer Potenciometro\r\n");
    enviar_CadenaUART("Seleccione una opcion: ");
}

// Interrupción de recepción UART
ISR(USART_RX_vect) {
    char dato = UDR0;  // Leer el dato recibido
    
    // Eco del carácter recibido
    enviar_CaracterUART(dato);
    
    // Si estamos en el menú principal, procesar las opciones
    if (ModoActual == 0) {
        if (dato == '1') {
            ModoActual = 1; // Cambiar a modo enviar datos ASCII por USART
            enviar_CadenaUART("\r\nModo Enviar ASCII activado\r\n");
            enviar_CadenaUART("Ingrese caracteres (M para volver al menu):\r\n");
        } 
        else if (dato == '2') {
            ModoActual = 2; // Cambiar a modo leer potenciómetro
            LeerPotentiometro = 1; // Activar la lectura continua
            enviar_CadenaUART("\r\nModo Leer Potenciometro activado\r\n");
            enviar_CadenaUART("(Presione M para volver al menu)\r\n");
        }
    }
    // Si estamos en modo enviar datos por USART, mostrar en LEDs
    else if (ModoActual == 1) {
        mostrar_CaracterEnLEDs(dato);
        
        // Si el usuario presiona '1' o 'L'/'l' durante el modo ASCII, volver al menú
        if (dato == '1' || dato == 'L' || dato == 'l') {
            ModoActual = 0; // Volver al menú principal
            LeerPotentiometro = 0; // Asegurar que se detiene la lectura del potenciómetro
            mostrarMenu();
        }
    }
    // Si estamos en modo leer potenciómetro y se presiona 'L' o 'l'
    else if (ModoActual == 2 && (dato == 'L' || dato == 'l')) {
        ModoActual = 0; // Volver al menú principal
        LeerPotentiometro = 0; // Detener lectura del potenciómetro
        mostrarMenu();
    }
    
    // Si es Enter, completar la cadena
    if (dato == '\r' || dato == '\n') {
        if (dato == '\r') {  // Solo enviar nueva línea si recibimos CR
            enviar_CaracterUART('\n');
        }
        
        // Marcar la cadena como completa
        Buffer[Indice] = '\0';
        Ccompletada = 1;
    }
    else if (Indice < MAX_BUFFER - 1) {
        Buffer[Indice++] = dato;
    }
}