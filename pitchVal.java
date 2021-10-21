public class pitchVal {
    public static void main(String[] args) {
        for(int i = 1; i <= 12; i++) {
            System.out.println("const float bendVal" + i + "[] = {" );
            for(double j = -63; j <= 63; j++) {
                double bendF = j / 63;
                bendF *= i; //bendFactor
                bendF /= 12;
                bendF = Math.pow(2,bendF);
                if(j > -63 && j < 0 || j > 0 && j < 63) {
                    bendF += 0.006;
                }
                System.out.println(String.format("%.3f", bendF) + ",");
            }
            System.out.println("};\n");
        }
    }
}