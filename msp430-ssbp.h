#ifndef CONFIG_H_
#define CONFIG_H_

#define F_CPU  16000000     // use calibrated 16MHZ clock

#ifdef __MSPGCC__
#define _enable_interrupts() __bis_status_register(GIE)
#define _disable_interrupts() __bic_status_register(GIE)
#endif

#endif

/*
 * ringbuffer.h - template for a circular buffer
 *
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

/**
 * ringbuffer - a template based interrupt safe circular buffer structure with functions
 */

template<typename T, int MAX_ITEMS>
struct ringbuffer {
    volatile int head;
    volatile int tail;
    volatile T buffer[MAX_ITEMS];

    /**
     * empty() - checks the buffer for data
     *
     * returns true if empty, false if there is data
     */
    inline bool empty() {
        bool isEmpty;

        _disable_interrupts(); // prevent inconsistent reads
        isEmpty = (head == tail);
        _enable_interrupts();

        return isEmpty;
    }

    /**
     * push_back() - append a byte to the buffer is possible
     *               assumed to be called from the recv interrupt
     */
    inline void push_back(T c) {
        int i = (unsigned int) (head + 1) % MAX_ITEMS;
        if (i != tail) {
            buffer[head] = c;
            head = i;
        }
    }

    /**
     * pop_front() - remove a value from front of ring buffer
     */
    inline T pop_front() {
        T c = -1;

        _disable_interrupts(); // disable interrupts to protect head and tail values
        // This prevents the RX_ISR from modifying them
        // while we are trying to read and modify

        // if the head isn't ahead of the tail, we don't have any characters
        if (head != tail) {
            c = (T) buffer[tail];
            tail = (unsigned int) (tail + 1) % MAX_ITEMS;
        }

        _enable_interrupts(); // ok .. let everyone at them

        return c;
    }
};

typedef ringbuffer<uint8_t, 16> ringbuffer_ui8_16; // ringbuffer, max of 16 uint8_t values
typedef ringbuffer<uint8_t, 32> Ringbuffer_uint8_32; // ringbuffer, max of 32 uint8_t values

#endif /* RINGBUFFER_H_ */

#ifndef HW_SERIAL_H
#define HW_SERIAL_H

template<typename T_STORAGE>
struct Serial {
    T_STORAGE &_recv_buffer;

    /**
     * init - setup the USCI UART hardware for 9600-8-N-1
     *        P1.1 = RX PIN, P1.2 = TX PIN
     */
    inline void init() {
        P1SEL = BIT1; // P1.1=RXD, P1.2=TXD
        P1SEL2 = BIT1; // P1.1=RXD, P1.2=TXD

        UCA0CTL1 |= UCSSEL_2; // use SMCLK for USCI clock
        UCA0BR0 = 130; // 16MHz 9600
        UCA0BR1 = 6; // 16MHz 9600
        UCA0MCTL = UCBRS1 + UCBRS0; // Modulation UCBRSx = 3
        UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
        IE2 |= UCA0RXIE; // Enable USCI0RX_ISR interrupt
    }

    inline bool empty() {
        return _recv_buffer.empty();
    }

    inline int recv() {
        return _recv_buffer.pop_front();
    }

    void xmit(uint8_t c) {
        while (!(IFG2 & UCA0TXIFG))
            ; // USCI_A0 TX buffer ready?

        UCA0TXBUF = (uint8_t) c; // TX -> RXed character
    }

    void xmit(const char *s) {
        while (*s) {
            xmit((uint8_t) *s);
            ++s;
        }
    }

};

#endif /* HW_SERIAL_H */

