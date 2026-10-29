// Decompiles functions for tools/ghidra/decompile.sh (script args: symbols-prefix outfile addr...):
// names every function and data object the tables name, then prints each function's C.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;

public class Decompile extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String root = args[0];  // symbols/<bin>.
        PrintWriter out = new PrintWriter(new FileWriter(args[1]));
        ghidra.program.model.symbol.SymbolTable st = currentProgram.getSymbolTable();
        for (String kind : new String[] {"funcs", "data"}) {
            BufferedReader br = new BufferedReader(new FileReader(root + kind + ".tsv"));
            String line;
            while ((line = br.readLine()) != null) {
                if (line.startsWith("#") || line.startsWith("addr")) continue;
                String[] f = line.split("	");
                if (f.length < 3) continue;
                String name = kind.equals("funcs") ? f[3] : f[2];
                if (name.startsWith("FUN_") || name.startsWith("D_")) continue;
                name = name.replace('@', '_');
                try {
                    Address a = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(Long.parseLong(f[0], 16));
                    Function fn = kind.equals("funcs") ? getFunctionAt(a) : null;
                    if (fn != null) fn.setName(name, ghidra.program.model.symbol.SourceType.USER_DEFINED);
                    else st.createLabel(a, name, ghidra.program.model.symbol.SourceType.USER_DEFINED);
                } catch (Exception e) { }
            }
            br.close();
        }
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        for (int i = 2; i < args.length; i++) {
            Address a = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(Long.parseLong(args[i], 16));
            Function f = getFunctionAt(a);
            if (f == null) {
                // the analysis missed it: disassemble and create it (the session is read-only)
                disassemble(a);
                f = createFunction(a, null);
            }
            if (f == null) f = getFunctionContaining(a);
            if (f == null) { out.println("// no function at " + args[i]); continue; }
            DecompileResults r = di.decompileFunction(f, 120, monitor);
            out.println("// ==== " + args[i] + " " + f.getName());
            out.println(r.getDecompiledFunction() != null ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
        }
        out.close();
    }
}
