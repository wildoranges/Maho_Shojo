define i32 @callee(i32 %0) {
entry:
  %1 = alloca i32                 ; initialize return_value
  %2 = alloca i32                 ; %2 = &a
  store i32 %0, i32* %2           ; initialize a
  %3 = load i32, i32* %2
  %4 = mul i32 2, %3              ; %4 = 2 * a
  store i32 %4, i32* %1           ; store return_value
  %5 = load i32, i32* %1          ; return
  ret i32 %5
}
define i32 @main() {
entry:
  %0 = alloca i32                 ; initialize return_value
  store i32 0, i32* %0
  %1 = call i32 @callee(i32 110)  ; call func callee
  store i32 %1, i32* %0           ; store callee's return_value
  %2 = load i32, i32* %0          ; return
  ret i32 %2
}