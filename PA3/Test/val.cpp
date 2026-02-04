// val.cpp
#include "val.h"
#include <cmath>

// Arithmetic: +, -, *, /
Value Value::operator+(const Value& op) const {
    if (T == VINT   && op.T == VINT)   return Value(Itemp + op.Itemp);
    if (T == VREAL  && op.T == VREAL)  return Value(Rtemp + op.Rtemp);
    return Value();
}

Value Value::operator-(const Value& op) const {
    if (T == VINT   && op.T == VINT)   return Value(Itemp - op.Itemp);
    if (T == VREAL  && op.T == VREAL)  return Value(Rtemp - op.Rtemp);
    return Value();
}

Value Value::operator*(const Value& op) const {
    if (T == VINT   && op.T == VINT)   return Value(Itemp * op.Itemp);
    if (T == VREAL  && op.T == VREAL)  return Value(Rtemp * op.Rtemp);
    return Value();
}

Value Value::operator/(const Value& op) const {
    if (T == VINT && op.T == VINT) {
        if (op.Itemp == 0) return Value();
        return Value(Itemp / op.Itemp);
    }
    if (T == VREAL && op.T == VREAL) {
        if (op.Rtemp == 0.0) return Value();
        return Value(Rtemp / op.Rtemp);
    }
    return Value();
}

// Integer remainder (mod)
Value Value::operator%(const Value& op) const {
    if (T == VINT && op.T == VINT) {
        if (op.Itemp == 0) return Value();
        return Value(Itemp % op.Itemp);
    }
    return Value();
}

// Relational: ==, !=, >, <, >=, <=
Value Value::operator==(const Value& op) const {
    if (T != op.T) return Value();
    switch (T) {
        case VINT:    return Value(Itemp == op.Itemp);
        case VREAL:   return Value(Rtemp == op.Rtemp);
        case VSTRING: return Value(Stemp == op.Stemp);
        case VCHAR:   return Value(Ctemp == op.Ctemp);
        case VBOOL:   return Value(Btemp == op.Btemp);
        default:      return Value();
    }
}

Value Value::operator!=(const Value& op) const {
    Value eq = (*this) == op;
    if (eq.T == VBOOL) return Value(!eq.Btemp);
    return Value();
}

Value Value::operator>(const Value& op) const {
    if (T != op.T) return Value();
    switch (T) {
        case VINT:    return Value(Itemp >  op.Itemp);
        case VREAL:   return Value(Rtemp >  op.Rtemp);
        case VSTRING: return Value(Stemp >  op.Stemp);
        case VCHAR:   return Value(Ctemp >  op.Ctemp);
        case VBOOL:   return Value(Btemp >  op.Btemp);
        default:      return Value();
    }
}

Value Value::operator<(const Value& op) const {
    if (T != op.T) return Value();
    switch (T) {
        case VINT:    return Value(Itemp <  op.Itemp);
        case VREAL:   return Value(Rtemp <  op.Rtemp);
        case VSTRING: return Value(Stemp <  op.Stemp);
        case VCHAR:   return Value(Ctemp <  op.Ctemp);
        case VBOOL:   return Value(Btemp <  op.Btemp);
        default:      return Value();
    }
}

Value Value::operator>=(const Value& op) const {
    if (T != op.T) return Value();
    switch (T) {
        case VINT:    return Value(Itemp >= op.Itemp);
        case VREAL:   return Value(Rtemp >= op.Rtemp);
        case VSTRING: return Value(Stemp >= op.Stemp);
        case VCHAR:   return Value(Ctemp >= op.Ctemp);
        case VBOOL:   return Value(Btemp >= op.Btemp);
        default:      return Value();
    }
}

Value Value::operator<=(const Value& op) const {
    if (T != op.T) return Value();
    switch (T) {
        case VINT:    return Value(Itemp <= op.Itemp);
        case VREAL:   return Value(Rtemp <= op.Rtemp);
        case VSTRING: return Value(Stemp <= op.Stemp);
        case VCHAR:   return Value(Ctemp <= op.Ctemp);
        case VBOOL:   return Value(Btemp <= op.Btemp);
        default:      return Value();
    }
}

// Logical: &&, ||, !
Value Value::operator&&(const Value& op) const {
    if (T == VBOOL && op.T == VBOOL) return Value(Btemp && op.Btemp);
    return Value();
}

Value Value::operator||(const Value& op) const {
    if (T == VBOOL && op.T == VBOOL) return Value(Btemp || op.Btemp);
    return Value();
}

Value Value::operator!() const {
    if (T == VBOOL) return Value(!Btemp);
    return Value();
}

// String/Character concatenation
Value Value::Concat(const Value& op) const {
    if (T == VSTRING && op.T == VSTRING) {
        return Value(Stemp + op.Stemp);
    }
    if (T == VSTRING && op.T == VCHAR) {
        string s = Stemp; s.push_back(op.Ctemp);
        return Value(s);
    }
    if (T == VCHAR && op.T == VSTRING) {
        string s; s.push_back(Ctemp);
        s += op.Stemp;
        return Value(s);
    }
    if (T == VCHAR && op.T == VCHAR) {
        string s; s.push_back(Ctemp); s.push_back(op.Ctemp);
        return Value(s);
    }
    return Value();
}

// Exponentiation (floats only)
Value Value::Exp(const Value& op) const {
    if (T == VREAL && op.T == VREAL) {
        // SADAL rules: if base == 0, result = 0; if exponent == 0, result = 1
        if (Rtemp == 0.0)       return Value(0.0);
        if (op.Rtemp == 0.0)    return Value(1.0);
        return Value(std::pow(Rtemp, op.Rtemp));
    }
    return Value();
}
