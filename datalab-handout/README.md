# DataLab

**姓名**：魏剑宇

**学号**：PB17111586

**代码地址**：[Github](https://github.com/kaleid-liner/CSAPP/tree/master/datalab-handout)(在DDL之后我会将此仓库make public)

---

## Results

**Dev Env**：WSL Ubuntu 18.04

结果如下

![datalab](assets/datalab.png)

## Implementations

### bitXor

由简单的布尔代数知识可知，$x \oplus y = x \cdot \neg y + \neg x \cdot y ​$.

由于不能使用位或，进一步转化得$\neg(\neg(\neg x \cdot y) \cdot \neg(x \cdot \neg y))$

```c
 int bitXor(int x, int y) {                     
     return ~(~(x & ~y ) & ~(~x & y));            
 }                                              
```

### tmin

最小的32位补码数是1 << 31

```c
int tmin(void) {              
    return 1 << 31;           
}                    
```

### isTmax

最大的32位补码数是1 << 31 - 1，判断两个数是否相等可以用xor。只有当两个数完全相等时，$x \oplus y = 0$

```c
int isTmax(int x) {
    /* means x == max */
  
    int max = ~(1 << 31);
    return !(x ^ max);
}
```

### allOldBits

通过`0xAA << 24 | 0xAA << 16 | 0xAA << 8 | 0xAA`很容易的可以构造出一个奇数位都为1，偶数位都为0的数。之后，将待判定数和此数进行位与，即可将带判定数的偶数位置为0，再判断此两数是否相等即可。

```c
int allOddBits(int x) {
    int all_odd = 0xAA << 24 | 0xAA << 16 | 0xAA << 8 | 0xAA;
    return !(all_odd ^ (all_odd & x));
}  
```

### negate

这个直接由简单的布尔代数就可以得到。

```c
int negate(int x) {
    return ~x + 1;
}
```

### isAsciiDigit

这里我使用最简单的思路，即判定x是否在最小数和最大数之间，即`x - min >= 0 && x - max <= 0`。实现中只需要注意一下返回值必须是0或1。

值得一题的是，这里在相减的过程中由于x太大或太小发生溢出不会对结果产生影响。

```c
int isAsciiDigit(int x) {                                  
    int min = 0x2F;                                          
    int max = 0x39;                                          
    int neg_x = ~x + 1;                                      
    // overflow doesn't matter                               
    return ((neg_x + min) >> 31) & !((max + neg_x) >> 31);   
}                                                          
```

### conditional

首先构造一个值`mask`，它在`x`为0时各位皆为0，在`x`为1时各位皆为1。这个数还是很好构造的。

1. 将`x` bool化，即`x = !!x` ,使x不是0就是1.
2. `mask = ~x + 1`。这样，若x是0，mask是0，若x是1，mask是1111.....111111.
3. 这样，最后的结果就是`(y & mask) | (z & ~mask)`

```c
int conditional(int x, int y, int z) {              
    int bool_x = !!x;                                 
    int mask = ~bool_x + 1;                           
    return (y & mask) | (z & ~mask);                  
}                                                   
```

### isLessOrEqual

这题的思路非常简单，通过判定`y - x >= 0`即可。但需要注意溢出的问题，所以代码较为繁杂。

```c
int isLessOrEqual(int x, int y) {                                                       
    int neg_x = ~x + 1;
    int mask = 1 << 31;
    int x_is_neg = x >> 31;
    int y_is_neg = y >> 31;                                                                                                                                            
    return ((!((neg_x + y) & mask)) | (x_is_neg & !y_is_neg)) & (!(y_is_neg & !x_is_neg));
}                                                                                       
```

### logicalNeg

这题我的基本思路是将-x和x进行xor。这样，只有x是零，最高位才是0.

然而，上述只是理论上的，存在一个特殊值`int32.min` ，在取负时会发生溢出，需要单独处理这个例外。

```c
int logicalNeg(int x) {
    int neg_xor_x = (~x + 1) ^ x;
    return ((~neg_xor_x >> 31) & 1) & (~x >> 31);
}  
```

这里我通过并上一个`(~x >> 31)`来处理例外。这样若x为负，一定会返回0.

### howManyBits

这题相对较难，经过思考最适合的方法是二分法，即判断该数是否需要用16、8、4、2、1位表示，在结果中加上这些位数。代码如下所示，

```c
int howManyBits(int x) {
    /* binary search */
    int sign = x >> 31;

    int abs_x = (x & ~sign) | (~x & sign); // in fact, abs_x is not abs of x

    // 16 bits required to represent x?
    int bit16 = !!(abs_x >> 16); //boolize
    bit16 = bit16 << 4;
    abs_x = abs_x >> bit16;

    int bit8 = !!(abs_x >> 8) << 3;
    abs_x = abs_x >> bit8;

    int bit4 = !!(abs_x >> 4) << 2;
    abs_x = abs_x >> bit4;

    int bit2 = !!(abs_x >> 2) << 1;
    abs_x = abs_x >> bit2;

    int bit1 = !!(abs_x >> 1);
    abs_x = abs_x >> bit1;

    int bit0 = !!abs_x;

    return bit16 + bit8 + bit4 + bit2 + bit1 + bit0 + 1;

}
```

这里，abs_x指的是和表示x所需位数相同的正数。

### floatScale2

此题需要分情况讨论

- 当指数部分位255时，说明次数为NaN，不需要经过任何处理。
- 当指数在正常范围内时，直接将expo+1
- 若指数为0，此数有可能是一个非正规数。此时又有两种情况
  - fraction < (1 << 22): 此时直接将fraction左移一位即可
  - fraction >= (1 << 22):  此时fraction需左移一位，指数也需加1

```c
unsigned floatScale2(unsigned uf) {
    unsigned expo_mask = 0xFF << 23;
    unsigned expo = uf & expo_mask;
    unsigned frac_mask = 0x7FFFFF;
    unsigned frac = uf & frac_mask;
    if (expo == expo_mask) {
  
    }
    else if (expo == 0) {
        if (frac >> 22) {
            expo = 1 << 23;
        }
        frac = frac << 1;
    }
    else {
        expo = expo + (1 << 23);
    }
  
    uf = (uf & ~expo_mask) | expo;
    uf = (uf & ~frac_mask) | (frac & frac_mask);
    return uf;
}
```

### floatFloat2Int	

此题思路很简单，只不过分情况讨论较为繁琐。

首先记录下符号为，之后分为三种情况：

- 数太小时，即expo <= 126，直接取0即可。
- 如果数太大，即expo > 157，直接取0x80000000即可。
- 否则，通过对`frac`进行一味计算round后的int值。

```c
int floatFloat2Int(unsigned uf) {
    int sign = uf & (1 << 31);
    int expo = (uf >> 23) & (0xFF);
    int frac = uf & 0x7FFFFF;
    int res;
    if (expo <= 126)
        res = 0;
    else if (expo > 157)
        res = 0x80000000;  
    else {
        expo = expo - 0x7FFF;
        res = 1 << expo;
        if (expo >= 23)
            frac = frac << (expo - 23);
        else
            frac = frac >> (23 - expo);
        res = res + frac;
        if (sign)
            res = -res;
    }
    return res;
}  
```

### floatPower2

此题同样思路十分简单，只需通过x的大小计算expo和frac的值，主要的计算操作是移位。下面是代码，通过代码可以很明显的读清楚有哪些情况即各个情况的处理方法。

```c
unsigned floatPower2(int x) {
    int expo, frac;
    if (x > 127) {
        expo = 255;
        frac = 0;
    }
    else if (x <= 127 && x > -127) {
        expo = 127 + x;
        frac = 0;
    }
    else if (x <= -127 && x > -150) {
        expo = 0;
        frac = 1 << (x + 149);
    }
    else {
        expo = 0;
        frac = 0;
    }
    return frac + (expo << 23);
}  
```

