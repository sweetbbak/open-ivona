// Headless export of function inventory for the dc harness.
// Writes JSON: [{entry, mode, ranges:[[start,end_excl],...], name, thunk, external}]
//@category IVONA
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.lang.Register;
import java.io.*;
import java.math.BigInteger;

public class ExportFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File out = new File(args[0]);
        Register tmode = currentProgram.getRegister("TMode");
        long base = currentProgram.getImageBase().getOffset();
        StringBuilder sb = new StringBuilder("[\n");
        boolean first = true;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.isExternal()) continue;
            Address e = f.getEntryPoint();
            if (!e.isMemoryAddress()) continue;
            BigInteger t = currentProgram.getProgramContext().getValue(tmode, e, false);
            String mode = (t != null && t.intValue() == 1) ? "thumb" : "arm";
            if (!first) sb.append(",\n");
            first = false;
            sb.append(String.format("{\"entry\":%d,\"mode\":\"%s\",\"name\":\"%s\",\"thunk\":%b,\"ranges\":[",
                e.getOffset() - base, mode, f.getName().replace("\\", "\\\\").replace("\"", "\\\""), f.isThunk()));
            boolean rf = true;
            for (AddressRange r : f.getBody()) {
                if (!rf) sb.append(",");
                rf = false;
                sb.append(String.format("[%d,%d]", r.getMinAddress().getOffset() - base, r.getMaxAddress().getOffset() - base + 1));
            }
            sb.append("]}");
        }
        sb.append("\n]\n");
        try (Writer w = new FileWriter(out)) { w.write(sb.toString()); }
        println("exported to " + out);
    }
}
