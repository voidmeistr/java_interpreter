class Main {
    static int b = 50;
    static void run() {
        ifj16.print(b+"\n");
        if ( b >= 1) {
            b = b - 1;
            run();
        }
    }
}

