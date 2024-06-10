; RUN: opt < %s -passes=reduce-math,instcombine -S | FileCheck %s

; a^2 - 2ab + b^2 -> (a - b)^2
; AB = a - b
; S = AB * AB
define i32 @square_of_dif(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @square_of_dif(
; CHECK:         [[AB:%.*]] = sub i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[S:%.*]] = mul i32 [[AB]], [[AB]]
; CHECK-NEXT:    ret i32 [[S]]
;
  %a.sq = mul i32 %a, %a
  %a.2 = shl i32 %a, 1
  %b.minus.a.2 = sub i32 %b, %a.2 ; b - a*2
  %m = mul i32 %b.minus.a.2, %b ; (b - a*2) * b
  %s = add i32 %m, %a.sq ; (2a - b) * b + a*a
  ret i32 %s
}

; a^2 + b^2 + c^2 + 2ab + 2ac + 2bc -> (a + b + c)^2
; AB = a + b
; ABC = AB + c
; S = ABC * ABC
define i32 @square_of_sum3(i32 %a, i32 %b, i32 %c) {
entry:
; CHECK-LABEL: @square_of_sum3(
; CHECK:         [[AB:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[ABC:%.*]] = add i32 [[AB]], [[C:%.*]]
; CHECK-NEXT:    [[S:%.*]] = mul i32 [[ABC]], [[ABC]]
; CHECK-NEXT:    ret i32 [[S]]
;
  %a.plus.b = add i32 %a, %b
  %a.plus.b.2 = shl i32 %a.plus.b, 1
  %a.plus.b.2.plus.c = add i32 %a.plus.b.2, %c
  %a.plus.b.2.plus.c.c = mul i32 %a.plus.b.2.plus.c, %c
  %a.plus.b.sq = mul i32 %a.plus.b, %a.plus.b
  %sum = add i32 %a.plus.b.sq, %a.plus.b.2.plus.c.c
  ret i32 %sum
}

; a^3 - 3*a^2*b + 3*a*b^2 - b^3 -> (a - b)^3
; AB = a - b
; AB2 = AB * AB
; AB3 = AB2 * AB
define i32 @cube_of_difference(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @cube_of_difference(
; CHECK:         [[AB:%.*]] = sub i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[AB2:%.*]] = mul i32 [[AB]], [[AB]]
; CHECK-NEXT:    [[AB3:%.*]] = mul i32 [[AB2]], [[AB]]
; CHECK-NEXT:    ret i32 [[AB3]]
;
  %a.sq = mul i32 %a, %a
  %a.cu = mul i32 %a.sq, %a
  %a.3 = mul i32 %a, 3
  %a.sq.3 = mul i32 %a.3, %a
  %a.b.3 = mul i32 %a.3, %b
  %b.sq = mul i32 %b, %b
  %a.sq.3.plus.b.sq = add i32 %a.sq.3, %b.sq
  %s = sub i32 %a.b.3, %a.sq.3.plus.b.sq ; 3ab - (3a^2+b^2)
  %m = mul i32 %s, %b ; (3ab - (3a^2+b^2)) * b 
  %res = add i32 %m, %a.cu ; (3ab - (3a^2+b^2)) * b + a^3
  ret i32 %res
}

define i32 @cube_of_difference1(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @cube_of_difference1(
; CHECK:         [[AB:%.*]] = sub i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[AB2:%.*]] = mul i32 [[AB]], [[AB]]
; CHECK-NEXT:    [[AB3:%.*]] = mul i32 [[AB2]], [[AB]]
; CHECK-NEXT:    ret i32 [[AB3]]
;
  %a.3 = mul i32 %a, 3
  %a.3.minus.b = sub i32 %a.3, %b
  %a.3.minus.b.b = mul i32 %a.3.minus.b, %b
  %a.sq = mul i32 %a, %a
  %a.cu = mul i32 %a.sq, %a
  %a.sq.3 = mul i32 %a.3, %a
  %s = sub i32 %a.3.minus.b.b, %a.sq.3 ; (3a - b) * b - 3a^2
  %m = mul i32 %s, %b ; ((3ab - b) * b - 3a^2) * b
  %res = add i32 %m, %a.cu ; ((3ab - b) * b - 3a^2) * b + a^3
  ret i32 %res
}

define i32 @cube_of_difference2(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @cube_of_difference2(
; CHECK:         [[AB:%.*]] = sub i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[AB2:%.*]] = mul i32 [[AB]], [[AB]]
; CHECK-NEXT:    [[AB3:%.*]] = mul i32 [[AB2]], [[AB]]
; CHECK-NEXT:    ret i32 [[AB3]]
;
  %b.a = mul i32 %b, %a
  %b.a.3 = mul i32 %b.a, 3
  %b.sq = mul i32 %b, %b
  %b.a.3.minus.b.sq = sub i32 %b.a.3, %b.sq ; 3ab - b^2
  %a.sq = mul i32 %a, %a
  %a.cu = mul i32 %a.sq, %a
  %neg.a.sq.3 = mul i32 %a.sq, -3
  %s = add i32 %b.a.3.minus.b.sq, %neg.a.sq.3 ; (3ab - b^2) + -3a^2
  %m = mul i32 %s, %b ; ((3ab - b^2) + -3a^2) * b
  %res = add i32 %m, %a.cu ; ((3ab - b^2) + -3a^2) * b + a^3
  ret i32 %res
}

define i32 @cube_of_difference3(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @cube_of_difference3(
; CHECK:         [[AB:%.*]] = sub i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[AB2:%.*]] = mul i32 [[AB]], [[AB]]
; CHECK-NEXT:    [[AB3:%.*]] = mul i32 [[AB2]], [[AB]]
; CHECK-NEXT:    ret i32 [[AB3]]
;
  %b.sq = mul i32 %b, %b
  %a.sq = mul i32 %a, %a
  %a.cu = mul i32 %a.sq, %a
  %b.a.31 = sub i32 %b, %a
  %reass.add = mul i32 %b.a.31, %a
  %reass.mul = mul i32 %reass.add, 3
  %s = sub i32 %reass.mul, %b.sq
  %m = mul i32 %s, %b
  %res = add i32 %m, %a.cu
  ret i32 %res
}

; (a+b) * (a^2 - a*b + b^2) -> a^3 + b^3
; AA = a * a
; AAA = AA * a
; BB = b * b
; BBB = BB * b
; S = AAA + BBB
define i32 @cubes_sum(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @cubes_sum(
; CHECK:         [[AA:%.]] = mul i32 [[A:%.]], [[A]]
; CHECK-NEXT:    [[AAA:%.]] = mul i32 [[AA]], [[A]]
; CHECK-NEXT:    [[BB:%.]] = mul i32 [[B:%.]], [[B]]
; CHECK-NEXT:    [[BBB:%.]] = mul i32 [[BB]], [[B]]
; CHECK-NEXT:    [[S:%.]] = add i32 [[AAA]], [[BBB]]
; CHECK-NEXT:    ret i32 [[S]]
;
  %a.plus.b = add i32 %b, %a ; (b+a)
  %a.minus.b = sub i32 %a, %b ; (a-b)
  %a.minus.b.a = mul i32 %a.minus.b, %a ; (a-b)*a
  %b.sq = mul i32 %b, %b ;
  %s = add i32 %a.minus.b.a, %b.sq ; ((a-b) * a) + b*b
  %m = mul i32 %s, %a.plus.b ; (((a-b)a)+ b*b) * (b+a)
  ret i32 %m
}

; a\b * b\a -> (a == b)? 1 : 0
; C = (a==b)
define i32 @mul_fractions(i32 %a, i32 %b) {
entry:
; CHECK-LABEL: @mul_fractions(
; CHECK:         [[C:%.]] = icmp eq i32 [[A:%.]], [[B:%.]]
; CHECK-NEXT:    [[S:%.]] = zext i1 [[C]] to i32
; CHECK-NEXT:    ret i32 [[S]]
;
  %a.rem.b = urem i32 %a, %b ; a % b
  %q.b = sub nuw i32 %a, %a.rem.b ; a - (a % b) == q * b
  %d = udiv i32 %q.b, %a ; (a - (a % b)) \ a
  ret i32 %d
}