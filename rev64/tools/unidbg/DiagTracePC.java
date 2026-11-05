import com.github.unidbg.AndroidEmulator;
import com.github.unidbg.Module;
import com.github.unidbg.Symbol;
import com.github.unidbg.arm.backend.Backend;
import com.github.unidbg.linux.android.AndroidEmulatorBuilder;
import com.github.unidbg.memory.Memory;
import com.github.unidbg.pointer.UnidbgPointer;
import java.io.*;
import java.util.*;

public class DiagTracePC {
    static void wr(AndroidEmulator e, long addr, long val) {
        byte[] b = new byte[4];
        for (int i=0;i<4;i++) b[i]=(byte)((val>>(8*i))&0xFF);
        e.getBackend().mem_write(addr, b);
    }
    public static void main(String[] args) throws Exception {
        System.load("/tmp/opencode/unidbg/unicorn2-extract/natives/linux_64/libunicorn.so");
        AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
                .setProcessName("com.cnvcs.junqi")
                .setRootDir(new File("/tmp/opencode/unidbg/rootdir"))
                .addBackendFactory(new com.github.unidbg.arm.backend.Unicorn2Factory(false))
                .build();
        for (String d : new String[]{"libc.so","libstdc++.so","libm.so","libdl.so","libGLESv1_CM.so","libGLESv2.so"}) {
            File f = new File("/tmp/opencode/unidbg/rootdir/system/lib/" + d);
            if (f.exists()) try { emulator.loadLibrary(f, false); } catch(Throwable t){}
        }
        Module so = emulator.loadLibrary(new File("/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so"), false);
        long base = so.base; Memory mem = emulator.getMemory();
        System.out.println("junqi base=0x" + Long.toHexString(base));
        String[] gnames = {"__stack_chk_guard","__sF","_toupper_tab_","_tolower_tab_","_ctype_","__dso_handle"};
        long[] goff = {0x15b488,0x15b4bc,0x15b51c,0x15b620,0x15b764,0x15b7d8};
        for (int i=0;i<6;i++) {
            Symbol sym = null; try { sym = so.findSymbolByName(gnames[i], true); } catch(Throwable t){}
            if (sym != null) wr(emulator, base+goff[i], sym.getAddress());
            else { int sz=(gnames[i].equals("__sF")?0x400:(gnames[i].equals("__stack_chk_guard")||gnames[i].equals("__dso_handle")?16:256));
                   UnidbgPointer bp=mem.mmap(sz,7);
                   if (gnames[i].equals("__stack_chk_guard")) bp.setInt(0,0x0A0B0C0D);
                   if (gnames[i].equals("__dso_handle")) bp.setLong(0,base);
                   if (gnames[i].equals("_toupper_tab_")||gnames[i].equals("_tolower_tab_")) for(int k=0;k<256;k++) bp.setByte(k,(byte)k);
                   if (gnames[i].equals("_ctype_")) bp.setByte(0,(byte)0x04);
                   wr(emulator, base+goff[i], bp.toUIntPeer()); }
        }
        UnidbgPointer nop = mem.mmap(4, 7); nop.setShort(0,(short)0x4770);
        List<String> lines = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new FileReader("relocs.tsv"))) { String l; while((l=br.readLine())!=null) lines.add(l); }
        for (String l : lines) {
            String[] p = l.trim().split("\\s+"); if (p.length<2) continue;
            long off=Long.parseLong(p[0],16); String name=p[1];
            if (off>=0x15b488 && off<=0x15b7d8) continue;
            if (name.equals("__aeabi_atexit")||name.equals("__cxa_atexit")||name.equals("__stack_chk_fail")) { wr(emulator, base+off, nop.toUIntPeer()|1); continue; }
            Symbol sym=null; try{ sym=so.findSymbolByName(name,true);}catch(Throwable t){}
            wr(emulator, base+off, sym!=null?sym.getAddress():(nop.toUIntPeer()|1));
        }
        long dataEnd=(base+0x162e88+0xfff)&~0xfffL;
        long mapEnd=base+0x200000;
        if (mapEnd>dataEnd){ try{ UnidbgPointer ex=mem.mmap((int)(mapEnd-dataEnd),7); for(int i=0;i<(int)(mapEnd-dataEnd);i++) ex.setByte(i,(byte)0);}catch(Throwable t){} }
        byte[] bx = new byte[2]; bx[0]=(byte)0x70; bx[1]=(byte)0x47;
        emulator.getBackend().mem_write(base+0x11af58, bx);
        long[] inits={0x3dae5,0x6ef21,0xafbd9,0xafc85,0xaffd5,0xb01fd,0xb06f1,0xb1d69,0xb222d,0x105e8d,0x10a58d,0x11c10d};
        for(long a: inits){ try{ so.callFunction(emulator,a);}catch(Throwable t){} }
        System.out.println("init_array done");
        UnidbgPointer dst = mem.mmap(16, 7);
        for(int i=0;i<16;i++) dst.setByte(i,(byte)0);
        long daddr = dst.toUIntPeer();
        // 记录 add r3,pc 的 r3 值：code hook on 0xa3802
        final long b = base;
        emulator.getBackend().hook_add_new(new com.github.unidbg.arm.backend.CodeHook() {
            public void hook(com.github.unidbg.arm.backend.Backend backend, long address, int size, Object user) {
                if (address == (b+0xa3802)) {
                    long r3 = backend.reg_read(unicorn.ArmConst.UC_ARM_REG_R3).longValue();
                    long pc = backend.reg_read(unicorn.ArmConst.UC_ARM_REG_PC).longValue();
                    System.out.println("HOOK@0xa3802 (add r3,pc 执行前): r3=" + Long.toHexString(r3) + " pc=" + Long.toHexString(pc));
                    // 期望 r3=literal(0xb6072), add r3,pc 后 = pc(unmask)+literal
                }
            }
            public void onAttach(com.github.unidbg.arm.backend.UnHook h) {}
            public void detach() {}
        }, b+0xa37fc, b+0xa3818, null);
        try {
            Object r = so.callFunction(emulator, 0xa37fc|1, daddr);
            System.out.println("0xa37fc -> " + r);
            long stored = rd(emulator, daddr);
            System.out.println("  [dst] stored vtable=0x" + Long.toHexString(stored));
            System.out.println("  期望不mask PC: base+0x159880=0x" + Long.toHexString(b+0x159880));
            System.out.println("  期望mask   PC: base+0x15987e=0x" + Long.toHexString(b+0x15987e));
        } catch(Throwable t) { System.out.println("THREW " + t.getClass().getSimpleName() + ": " + t.getMessage()); }
        emulator.close();
    }
    static long rd(AndroidEmulator e, long addr) {
        byte[] b = e.getBackend().mem_read(addr,4);
        long v=0; for(int i=3;i>=0;i--) v=(v<<8)|(b[i]&0xFF);
        return v;
    }
}
