#!/usr/bin/env python3
"""Apply the VDR-Live-aligned Phase 61 EPG Genre patch locally.

The patch is embedded so a mobile workflow only needs git pull and one Python
command. The script performs git apply --check first and never commits or pushes.
"""

from __future__ import annotations

import base64
import subprocess
import sys
import zlib
from pathlib import Path

EXPECTED_BRANCH = "feature/phase61-metadata-genre-browser"
PATCH_B85 = r"""c-pl**>>B=vG4qfHs>6RyaXsN619yy@ksPoCmzX>G@d;89I6R4Da0UvV3EX0=5Oww+#kDD)eWGraFY`yLy$mMb?seM-LUP<X3R9_PR#he<%eQ_A!2UxnD0lSwf~FoLUHBU;^kHZe&ob{xcn&GK!j1(@hmpo9j0NL=4<9^bUK|^&G(nTFmre?G7ecsUf{;m_g&_AkqF~)F#kF9pCfVM@+fj<j>Tig_gW2>{xz?z@5Z<H?CSddhDC`Ji%CkrWR8<h5OD4-gcnDwsR6dKaWN0)BAz(5G3C}b;o10A-HexkFeD<l8HE0mV~Y^K`=K*;Jnl}OxSI$=NA3CHg1gQiMC>F?Twz3h5?W&N&GBp_r)UzzJQjxKdow`mSpu-yh}7_>!2gr5Cp<Qi0LY}TZQ#Jnrup*z({JND*80i#$zXdx+dSk8Q#=9UJ>w_(-<}BO=9ENe#{9+i{Ke9I>hA%$o$tpp*ekHD4+55AfHy5v+@7(=t=M@Ih^`0!UHo=?JHEc3++Y1VzPo?_>*w}a^Gq7U@yIv<V}Q#jGlozAI?Hjz3VZnEgW7Cm6$Altb;Ym3cKAIgke#y`cO!A8kFY`xW_ama32K6e%Qd6!0zU-v$p)C&0Q+hBE7*is@$627j)>|o!@~`8Blx;`KRE7!{Hp{U#rEl`<He(cNz76^dLP56o|N&yV2Cn481!B%WBmIw|0-Z|=FC_t^^Yr?ou9MjlO3916HWH^Et3xLSFnG21oUS%Vha$n6F_WewsWNa$}JuLG(l%y|NU=9uBoP5;LYY)1uj!U{=Ebl(cR`*gOxywDk|M5A+hsEHJCD0=HJUi(A^sPbww9`0)kok<I0X&?XKk~Uffbx+w2{xT+<k7e|Tc_N1*=U&=~Yyr~iNDNKx$zbTld1f&vRNP}(M}Y^BWeP>A;0Jvk}RZ&oa>>c;InvaK1#-kcYmd$r}22`}oovu0?`v_BQ$*{P=YwSqV<2x2Q1(d&vVc5)UAbcZ5NLXX8^A~cRl9=^3^Q$O56v1FJ>hsLl6hIw>s^bcNVn2O|_TuqlKrIH7Ogx=&qA^lYrRt?}A&5vJeIp#XM&Z9&vjRku*x17EjU{=k%YOb<~ww^il^-VL&xc>HS5nVM9){ZEMX~pv*Gpms7$X;sj>taD^?ycg$whmZu-#X}$^NkMAQg><CyB8UE)}vVPoob96WTz5C1KXv-(7-UT4EhI<=ydSX?`LsEya+3a1;I;z)+HdF@<_a8w)&>|cJt!n_~NgvklT(A<wmP#^xJJi-UP{p%K&op>H|J?wcC*A>KJdw@9)R(@N)cjcJ+Z>-`unD->>fO@0er1Odx%lSdZNEggdb~kT^-}6E9gzMOY|jZmwBnd`qim(gzYW`{~$7QeF#Ojz5ioqz|_@ziv}b<$%9`9N&(aR_2_qE(Z#L@2@X4##?tlz{Nf5*OCyCo+eLFK0@S=n4DDoJRcQVQI+A%ExY>V`sQ|ATO*b*w$v9R%}L>Vo5=!6$K4tkd-q@N<mzhr&mQQQq5$KN;L32vB=5{BELZH=i67bJm!FyD{}M{Zdr0M7)Z1QWD{b)BKk@&s@YTO1M9WUjs=@5!tvcK;?yAF~zxGE*#u4kt3)~=>Z1ukZ6@j3iSskj=Q&AqLHw)0#rd`*W32SM&LYCj>X>nTcCmd>Zx3=dpV7bQ6Cy~$7mcscSnay6Iu*V@>pPL*6Vu(IyPvv$dW#1+?D)x-kG`4GIC=-cOWR?0};(hbLvI(+Ng47xz=X>#!B4w{;WI=*C$<ie10JbB_*uyo=b*z}^V<jZ1c%{&&u`nE=QOS#DRG;}=tTCmfG$zf6V5KG<s(97Zm*VEy=v57%b5;bG@oe6$wx-UfH8)GpNYy;Ite<p^XOGug)py!eHp`Zl1;B-@NkMO6U3L6yu&*77(=CkXEJR-(IQyZ9;ykEHLkZsp&Mnm7&O&spAb=RS<wzB2w;e6x>$amQ3xetjlAuG`WcBz$L;<%Tsf1(_IE`ZQ5`@L((WRL2#KkV`?;u|P`3_4l8?)JiWA;C6)T58HmGHMB@?8kSAArV3Pef6UV5Gx$Ho)JC7?Qdiwg>{FvyngZ*m}Hk{=htUbRgSZ{lU=aAH#YAybtoa5~Sr=kH8!Gy>f8GB_E)9d~y~}+-{`lHr6Ping}~KUgEkz9JXu9XT%diV&Qm>H^)u@vbEqZkOfxoryztq1A#ofgt(EW@=;tA6PPPZ0iWZ*2kxbuT5VNrX#h{cg2?t`8v1U<3>9+$$%lg-=6B3Wzk--^uMV@A*cDZ%s3qx_{nEs;1l2p5FrFWwde@nI2fWEgS|38viGrJ=5=l0h<Qy*tH>%ohpOx{vg5Od{WCPEI0xWI#3;So8abMX+al|b27x&<frQ7JEZq5B>7WK3++%(FwJ~MNrlNrUDb%oWXf4R){CuozojJjeG#LE^$!)Yk^H?T-^rQ+$y%nt#c^s!0@Z*v{M%9_umif_$6z6)8WrgHvbhhyC1V6@YrV#Nr1;+_6c&)5(tazg>}Cz#oHXMTd+?LsqNRoR6m)Ef<k$1zG(*llVUI35Wvhn1{8$>ROW3zAs&jMH0+>|><2(?32o`h(4MFVc;X6iZic+^;acvvu%#cb_$Dp^H{kFN2j0%9^Mb;-YY)ZEqAIHaiIG)Gjb3$0$g<Ti8x1QBVS`3TaE=KO3E5d%a>}Z{O->mnJ*iCkR0{xTjq1X{GNB4*EvFx1A~HK3uin0(G6-_|uSXVx)!w*P2*7ihl$pz0)H{1hEk)D}|OVAlQ$3Xi5W5QBG==ROFx4SIRaQ3*Vze>5{&x{0L-mfxQxyt!+J4uZ*`EuT{ibjYlP(3@~S-Px$%dL&{$OI6Vy$FNOM@hl?cm-4DNsu+?s`>cR@DiXHA=3YTl?)Ii@O$_Lv{7%wly6fDjX`RG0lH><<RPgFoIrE1#_?)TClJfdhum)G-M{!`5`h2jFMn3vdmH(jw}Z?SsU4S}daF}sj48#Rq)^><ueiS@tHL0gJsa6CYf$jd5`NO`7W`QV4|U03BJQA>-UQ5=HHVhrJt@6|}`6_Q#bp<N|29Mp;Au#dObbig2J#1Va}LfgR*H_;L~&BJ$ts_lYp2U*t#+YVCh)uPtvGgW#dguS6rtsjYj<aI*%O_X&pwIeWl9Ln5_K790h@Nq4k@vO#2k;GUD--FiaQ4b#u=z>4gz_w9?Yd|C6gLZP@dKdiP|He?f=jaY4y2BhD?(6JdhJ3-ldY2*=-=GxVzkduJ*9AzN$1Kk?gpVQO`Q*$%kV_~qWY-mLjRPn0ZJmRWoN`o}@;5N-@R7T5;>~HA&dO;%@-Y5`-TIJx`mVjpB*YhCm&NGF=nt_Nr7PURc!;a09=cX^Jng5cyhHheUZAXOc@i_cX|NL@hz)_PwifMqgr}@p8Q!i1Ywz5rZme8@u@F6IYY|_8sDaBI)=2lk+Fb|oxeGjrfY5#2n|Frp?Fwx{v1y0G%ZimMyS4CYi?U|=t-$roycz*)M}ph{tO2iD2DL5I4b!M<57oY9W(i7E@I^NZgB`+uy&6?#vvgxW=5B$o)njGQ=|b@L;#OP!R0mxKz2+{Uu@i%LzX8CquQzLqV->Z%wPV$kxy{I`>PS{qN3v(O&QnPrQF_p52W2QuR%)YcIdXiBDpm|!qlaOyPJs1rQV9rEjD*cJLFxA?uHjWS*)~cG;-`z}8n>3$v|zm0u=Ty|OgNot3k%MMt`xvc{3yaY$*q{Bw{B>~@H$m1RZDzV?O3;p$<ewk+YS4rXU!6--#%aBhV(8QziQj#o>dRMp07H6T7lnX^Z2Zp#q>WLmE&k>W|SpHHLG1JPd^Y|Yv#BDo8g6pqHaG(Gtvx2jb?RI_M#*^TTm!|3x2S0=HUiE=<FUh%5<f(Y+;-l6`KxUwIneY!>=AjlZp&e9`V4p6Yb*cuL_RYG&PQ1zkO55#ih-K6EwQe#{{&zkFzC6c|&Jgu+q-XwqO{9kEnThq>lBjD;JE)4nn(-%iD722UB2EmIs*Ien?_4XqIK>GQ)+-Wqw)$el?laz@m9#e<F*{7PjO%407Z)T$*xl{g{IR4bsn-8^h7=<wBK}exYpmfN?7>Wb{>bIvWwLDG+v*&dD~?Ih4{d>!q`%vZ+cjD^3Q36Qhq!L%h~5;<Af?yNd#{RSysD$(~YrHe0jGxxk%6{rH8FTwxiHmEQEt4Pen=lF$Cp;21r=uX}vDh-y=Jv6q-c7a(zT_(3mA1JaBe2m?l<0u;>uwW(5~ug%l1O|bK?O^RdN!K!0a*RX&P{3VarID`nqK3?vGh(B@1#j|3726s@J*?OTs-|Wi3R>0+Ct|Z}=aJORN1C#G%4%TiLcPCfbNAwy4`A)-#US}E6cS&{~G<y|vUqRDRmfi`rh2^EjNsEbm9s8_~eK~&3+~x)-N|6bs99?S!w;n;!E_L4f$HoA7hs(hl-ZSir*QsN%Ne`V3lWh?gHle5u356^s_I)?nhy2p|R!R<dXgxv>Z{d0^GX*G@qHeH+)Z!ITDwlZ;q)caC14$+L(E+L(FMT?;6^ef*PMBtk%|dCwgxYCiizLFNCT0QHkz+Xl?g%>`n?cbfHy>DfxF^E+(r$Dz#2p!lRBNC~XaMw4zO_gvHJjN%^YUXufz$cU?6sySCrBLzT{6{_1+$PUXMUJ=H2_JKpp|FtnmB!)Q!BGVDmNXzTMaqieX{v(lcj>f_m`mFoRU(MoGgDKn0T?=ByygFyQCcimL0cM3QUz<FRuFc6`K`JX0<RqlJ#;*8`SwgO9(X(7B1t@zpOaXD@!Z<<^Tr{Jo4s}s&pe3w{C_iV&xWGvgPewbdhu}J?k;_JymV1O%$@a^!Tt8WrmX>Gi4T-onT7I=&(2;ZKk~Ex3qccaORa=8M}0>Lcp?pE85pRKHC&ANvTR@VRvCe(AZ(L3Q)U1$iTD<1Zwkmh`Z|a2|urW@LAl)lGQGVW)89lm*~u3#es-Xyr}`g7l`I)Bo5|4c49%H3e}WU%%dY3?BU@7_u3jv^FKTUg$)@qx9Nu7D}m>$nk5ah+Lb8|*V;0T&h~=`M5+wYhsP)qd4U@@9(b(9J+K;!yYzx0Q{GKS-ys6Kf`c9TUhYX?K3I>SZKF{|!j(=3mfR@3NpnuovLP{^|5!KAJ4su^ynh5y(*cXnj6j$ucCc1KNh?S=4Z*D3SH=@&AcuH~hmIh9L3Jj8S+*<H3(^pyC@%3J5S-iyh$ki%BE+aQ6&6o01BQskL+T83bP#ScfrQ|KkL@&Vq3AX`_Y`!9VdbR2M9jYluN(&HfC-}))G3xxxWe5JWsDh|?^R>yFNGM|W#t7+p6p;xPUN7?!ZIZ-E?;A(39M)Wk3t51kf3NJ_aG~}YNSM6wV2ac6j;rNYVYemt)T|{;RkZ2V&r7EV9F7cD5cvlxh-JA1!Uh5<Ow9kJfKYlAcutQ{{A!WmD|xNpy+qmdqyX1YE{7=WMc4;n|4H+Ay!b150Zn2Zo?dG!bK^_8dT&A4+bulB8cH3NP;sffi(+8<TxL+?WqT)dkrZsyo*AfgxGZ;pmx}0zk>t&3C@v36w7oQ+%zqy=s|9AJ(SE3GWcbl8D%WOoCjTYHPbNKK9Gxflr1cPH7a&nz?67=F?HsNpJ=j5<!`JZFY}9rdDvwal$F|!MQvahmt)O12xL4*;;PH6gGZIY?fn@@Zk`Cp`E8bN0m2-kF1y2ju%K)ObaYPhI2SSi)Yh{AzYg=Io=%aH+<3ZTBXR0L2z%raQ|Fdtl%P}Z5NrM`_V*oTpv~jN=~_S;7&a4ZjIk7zXdKWF7Fh{3i~~k)Aw9rt9{Os?D(9%W-a7{}CO%xqSRA$54~+h$4><$GCV6xZ<&ZAcIVTuM&=jYF*-jL=d<i3gatJq2)Is>d=n32{RS3;YmU|WPN{Okcf`}Cegrqd)7GrJaF!xmDr}e7_Nk?}Q;&`K15NC>Ku*N*&)O9oZ@B2;X=s)5&yZH>R(6DaIeAfj^>Cu<VTUel;1jF|L!|2fopr*5hdn)fnz&<eeV9{yg(^K7|@KAG<1c56^;%;g!hB`D)7O=1Om64qo*E}qcqVkIVFc<L`7~<oZQ^bv?0>a|1KTmywDl3A+(d6AX;r$bgRx9-Jfq`#IoD9^V(eVpk0CBML<*8{trFz-AlP$*T&wM8z>0@PsxRY%jXk*EQ_p>K;Q2MLW3hL7TCU4<V-XPP(Tl?1>()hZ=boDUwuJ_7QeaHz8XqY&*(pr}ul#c}?PR&&MB_-jV43^sM1)u_~4pZ!ap$Ljdwt}gsD%VV!zGHccFGmj#LGzT7$#O&%fX{TQ5)E6Aa@nf{r%-pHXM}MQC<x_1r<mt2d`R<$gB&tH-+P9TK$zo0e30g8l3dg&c+yag<XC@I1H<1E-yVw66Wf3*Z!LiDhLF7BN$DKH=s?GS68UEWSk4-qjSL+2S8&kTg@Ymil7+s;Lgy7M-2Ls7JRMEoSWhW^1sFXk74p3ZEFPwT2_d`?O0doh5-<Vhu@4I_Q1ueHjs;6E?ViFKWkZxWYcR8Bg`1_P=}%eYyf9d7LdgY$e2-QPh_jBHVAz352m=otd8g%w&tkMH4>!NPBo;J>tUlEW5JItt$3#!@RWpSg=}4wPp%l-8zc`Czkqty1VTI*ORc;Iq=mh)_3e(K8XAnowJ84%HqythpMZC2O2+!*JV#9|hk&!8Ian;m<Q_V=_dNfo8%6+|_33#XmgBcfEI7mRYW}2DkRcj0($ZGzf>LpkvOF<&-E@n$dJ%nU3&2FM<PC;O$ch>hy@2hV|ZA>1<@t<{>17f@nRt+ouLnU}=d{C&vvIFlukW|GBw_;50z&0pg$?PLLzo@ng4o5~GY~bj?7;dtG&$sM{lM@y~+C=%CCDL55(UFHZ(vvf61c{t0&<)AssSgIlssoI}Pw*25(}uJoL%9rohtXJT-|}Y?CJHP!yi8!S^rj{eu$b5O%0CL}N`0HIP}G8YmV^`TIrNfR#EP9wA`lbD{|AY)JvR"""


def run(root: Path, *args: str, data: bytes | None = None) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        args,
        cwd=root,
        input=data,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def main() -> int:
    root_result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if root_result.returncode != 0:
        sys.stdout.buffer.write(root_result.stdout)
        return root_result.returncode

    root = Path(root_result.stdout.decode("utf-8").strip())
    branch = run(root, "git", "branch", "--show-current")
    current_branch = branch.stdout.decode("utf-8").strip()
    if current_branch != EXPECTED_BRANCH:
        print(f"ERROR: expected {EXPECTED_BRANCH}, found {current_branch or '<detached>'}")
        return 1

    status = run(root, "git", "status", "--porcelain")
    if status.stdout:
        print("ERROR: working tree is not clean")
        sys.stdout.buffer.write(status.stdout)
        return 1

    patch = zlib.decompress(base64.b85decode(PATCH_B85.encode("ascii")))

    check = run(
        root,
        "git",
        "apply",
        "--check",
        "--whitespace=error-all",
        "-",
        data=patch,
    )
    if check.returncode != 0:
        print("ERROR: git apply --check failed")
        sys.stdout.buffer.write(check.stdout)
        return check.returncode

    applied = run(
        root,
        "git",
        "apply",
        "--whitespace=error-all",
        "-",
        data=patch,
    )
    if applied.returncode != 0:
        print("ERROR: git apply failed")
        sys.stdout.buffer.write(applied.stdout)
        return applied.returncode

    diff_check = run(root, "git", "diff", "--check")
    if diff_check.returncode != 0:
        print("ERROR: git diff --check failed")
        sys.stdout.buffer.write(diff_check.stdout)
        return diff_check.returncode

    print("PASS: Phase 61 VDR-Live Genre patch applied locally.")
    sys.stdout.buffer.write(run(root, "git", "status", "--short").stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
