class Main {
    static String str = "mystring";
    static void run() {
        str = str + "second string";
        ifj16.print(str + "\n");
        int a = ifj16.length(str);
        ifj16.print("length of " + str +" is:"+a+ "\n");
        String str2 = ifj16.substr(str, 2, 4);
        ifj16.print(str2 + "\n");
        int b = ifj16.compare(str, str2);
        if (b < 0) {
            ifj16.print("str is greter than str2 \n");
        }
        else {
            ifj16.print("str is lesser than str2\n");
        }
    }
}
