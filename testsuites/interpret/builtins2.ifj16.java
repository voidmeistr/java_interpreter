class Main {
    static void run() {
        String str = ifj16.sort("babafagay06");
        String str2 = ifj16.substr(str, 2, 5);
        int i = ifj16.find(str, str2);
        ifj16.print(str+"\n"+i+"\n");
        str = ifj16.sort("");
        i = ifj16.find(str, str2);
        ifj16.print(str+"\n"+i+"\n");
        if (i != 1) {
            if (i == 0) {
                {}
            } else if (i < 0) {
                str = "dfafads" + (i*i) + "sfa" + str;
                ifj16.print("i:= " + i + "\n"+str+ "\n");
                str = ifj16.sort(str);
                i = ifj16.length(str2);
                ifj16.print("here" + str + "\n" + i + "\n");
            } else {
                str2 = "dfafads" + (i+i*i) + "sfa" + str;
                str2 = ifj16.sort(str2);
                i = ifj16.length(str);
                ifj16.print(str2 + "\n" + i + "\n");
            }

        }
    }
}

