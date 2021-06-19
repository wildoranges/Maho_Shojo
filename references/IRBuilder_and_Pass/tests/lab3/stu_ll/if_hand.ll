define i32 @main() {
entry:
  %0 = alloca i32           ; initialize return_value
  store i32 0, i32* %0
  %1 = alloca float         ; initialize a = 5.555
  store float 0x40163851e0000000, float* %1
  %2 = load float, float* %1
  %3 = fcmp ugt float %2,0x3ff0000000000000   ; (a > 1)?
  br i1 %3, label %trueBB, label %falseBB
trueBB:
  store i32 233, i32* %0    ; return 233
  %4 = load i32, i32* %0
  ret i32 %4
falseBB:
  br label %end_entry
end_entry:
  %5 = load i32, i32* %0  ; return 0
  ret i32 %5
}