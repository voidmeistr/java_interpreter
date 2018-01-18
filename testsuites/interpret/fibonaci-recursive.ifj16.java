class Main {
    static void run() {
        int fib10 = Fibonachi.fib(10);
        ifj16.print("10th Fibonaci number is: "+ fib10 + "\n");
    }
}

class Fibonachi {
    static int fib(int n) {
        int result;
        int tmp;
        int dec1 = n - 1;
        int dec2 = n - 2;
        if (n > 1) {
            result = fib(dec1);
            tmp = fib(dec2);
            return result + tmp;
        }
        else {
            return 1;
        }
    }
}
