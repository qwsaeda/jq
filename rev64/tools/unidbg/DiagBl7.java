import com.github.unidbg.AndroidEmulator;
import com.github.unidbg.linux.android.AndroidEmulatorBuilder;
import com.github.unidbg.arm.backend.Backend;
import unicorn.ArmConst;
import java.io.*;

public class DiagBl7 {
    public static void main(String[] args) throws Exception {
        System.load("/tmp/opencode/unidbg/unicorn2-extract/natives/linux_64/libunicorn.so");
        AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
                .setProcessName("test")
                .addBackendFactory(new com.github.unidbg.arm.backend.Unicorn2Factory(false))
                .build();
        Backend be = emulator.getBackend();
        long base=0x10000000;
        Object[][] tests = {
            {"F000 F800 (target=PC+4)",     new int[]{0xF000,0xF800},  0},
            {"F7CA FC3D (0x7965a)",         new int[]{0xF7CA,0xFC3D}, -0x35786},
            {"F7F5 FD43 (0x790da)",         new int[]{0xF7F5,0xFD43}, -0xa57a},
            {"F00E F920 (0x30228)",         new int[]{0xF00E,0xF920},  0xe240},
        };
        for (Object[] t : tests) {
            String name=(String)t[0];
            int[] enc=(int[])t[1];
            int imm32=(Integer)t[2];
            long retAddr=base+4;
            long tgt=(retAddr+imm32)&0xFFFFFFFFL;
            // 映射 base 区 + 清零
            try { be.mem_map(base, 0x4000, 7); } catch(Throwable e){ System.out.println(name+": map base err "+e.getMessage()); }
            try { be.mem_write(base, new byte[0x4000]); } catch(Throwable e){}
            // 放 BL（little-endian）
            byte[] bl=new byte[4];
            bl[0]=(byte)(enc[0]&0xFF); bl[1]=(byte)((enc[0]>>8)&0xFF);
            bl[2]=(byte)(enc[1]&0xFF); bl[3]=(byte)((enc[1]>>8)&0xFF);
            be.mem_write(base, bl);
            // 放 bx lr 在目标（若未映射, 先映射清零）
            long tpg=tgt&~0xFFFL;
            try { be.mem_map(tpg, 0x1000, 7); } catch(Throwable e){}
            try { be.mem_write(tpg, new byte[0x1000]); } catch(Throwable e){}
            byte[] bx={0x70,0x47};
            try { be.mem_write(tgt, bx); } catch(Throwable e){ System.out.println(name+": bx@tgt失败 "+e.getMessage()); continue; }
            // 放 bx lr 在返回地址（让 BL 返回后停住）
            try { be.mem_write(retAddr, bx); } catch(Throwable e){}
            // LR sentinel（映射清零）
            try { be.mem_map(0x20000000, 0x1000, 7); } catch(Throwable e){}
            try { be.mem_write(0x20000000, new byte[0x1000]); } catch(Throwable e){}
            be.reg_write(10, 0x20000000L); // LR
            long land;
            String msg="";
            try {
                be.emu_start(base|1, 0x20000001L, 0, 10000);
                land=be.reg_read(ArmConst.UC_ARM_REG_PC).longValue();
            } catch(Throwable e) {
                land=be.reg_read(ArmConst.UC_ARM_REG_PC).longValue();
                msg=e.getClass().getSimpleName();
            }
            long lr=be.reg_read(ArmConst.UC_ARM_REG_LR).longValue();
            System.out.println(name+": 结束PC=0x"+Long.toHexString(land)+" LR=0x"+Long.toHexString(lr)+" ("+msg+")");
            // 正确: BL→tgt→bx lr→返回 retAddr→bx lr→LR=0x20000000(零,nop)→跑到 stop
            // 实际 stop 地址是 0x20000001, 若 BL 正确会执行 nop 直到 stop, PC≈0x20000000
            if (land==0x20000000L || (land>=0x20000000L&&land<0x20001000L)) System.out.println("   ✓ BL 正确 (返回到 retAddr 链, PC 在 LR sentinel 区)");
            else if (land==retAddr) System.out.println("   ✓ BL 正确 (返回到 retAddr=0x"+Long.toHexString(retAddr)+")");
            else System.out.println("   ✗ 异常 PC=0x"+Long.toHexString(land)+" (期望目标 0x"+Long.toHexString(tgt)+" 返回 0x"+Long.toHexString(retAddr)+")");
            // unmap 清理
            try { be.mem_unmap(base, 0x4000); } catch(Throwable e){}
            try { be.mem_unmap(tpg, 0x1000); } catch(Throwable e){}
            try { be.mem_unmap(0x20000000, 0x1000); } catch(Throwable e){}
        }
        emulator.close();
    }
}
