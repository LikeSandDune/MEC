#include "Parameter.h"

#include <iostream>

#include <mec_log.h>

namespace Kontrol {


static void throwError(const std::string id, const char *what) {
    std::string w = id + std::string(what);
    std::runtime_error(w.c_str());
}


// Parameter : type id displayname
Parameter::Parameter(ParameterType type) : Entity("", ""), type_(type), current_(0.0f) {
    ;
}

void Parameter::init(const std::vector<ParamValue> &args, unsigned &pos) {
    if (args.size() > pos && args[pos].type() == ParamValue::T_String) id_ = args[pos++].stringValue();
    else
        throwError("null", "missing id");
    if (args.size() > pos && args[pos].type() == ParamValue::T_String)
        displayName_ = args[pos++].stringValue();
    else throwError(id(), "missing displayName");
}

static const char *PTS_Float = "float";
static const char *PTS_Int = "int";
static const char *PTS_Boolean = "bool";
static const char *PTS_Percent = "pct";
static const char *PTS_Frequency = "freq";
static const char *PTS_Time = "time";
static const char *PTS_Pitch = "pitch";

std::shared_ptr<Parameter> createParameter(const std::string &t) {
    if (t == PTS_Float) return std::make_shared<Parameter_Float>(PT_Float);
    else if (t == PTS_Int) return std::make_shared<Parameter_Int>(PT_Int);
    else if (t == PTS_Boolean) return std::make_shared<Parameter_Boolean>(PT_Boolean);
    else if (t == PTS_Percent) return std::make_shared<Parameter_Percent>(PT_Percent);
    else if (t == PTS_Frequency) return std::make_shared<Parameter_Frequency>(PT_Frequency);
    else if (t == PTS_Time) return std::make_shared<Parameter_Time>(PT_Time);
    else if (t == PTS_Pitch) return std::make_shared<Parameter_Pitch>(PT_Pitch);

    std::cerr << "parameter type not found: " << t << std::endl;

    return std::make_shared<Parameter>(PT_Invalid);
}

void Parameter::createArgs(std::vector<ParamValue> &args) const {
    switch (type_) {
        case PT_Float:
            args.push_back(PTS_Float);
            break;
        case PT_Boolean:
            args.push_back(PTS_Boolean);
            break;
        case PT_Int:
            args.push_back(PTS_Int);
            break;
        case PT_Percent:
            args.push_back(PTS_Percent);
            break;
        case PT_Frequency:
            args.push_back(PTS_Frequency);
            break;
        case PT_Time:
            args.push_back(PTS_Time);
            break;
        case PT_Pitch:
            args.push_back(PTS_Pitch);
            break;
        default:
            args.push_back("invalid");
    }
    args.push_back(ParamValue(id_));
    args.push_back(ParamValue(displayName_));
}


// factory for all type creation
std::shared_ptr<Parameter> Parameter::create(const std::vector<ParamValue> &args) {
    unsigned pos = 0;
    std::shared_ptr<Parameter> p;

    if (args.size() > pos && args[pos].type() == ParamValue::T_String) p = createParameter(args[pos++].stringValue());
    try {
        if (p->type() != PT_Invalid) p->init(args, pos);
    } catch (const std::runtime_error &e) {
        // perhaps report here why
        std::cerr << "error: " << e.what() << std::endl;
        p->type_ = PT_Invalid;
    }

    return p;
}


std::string Parameter::displayValue() const {
    static std::string sNullString = "";
    return sNullString;
}

const std::string &Parameter::displayUnit() const {
    static std::string sNullString = "";
    return sNullString;
}


ParamValue Parameter::calcRelative(float f) {
    switch (current_.type()) {
        case ParamValue::T_Float : {
            float v = current_.floatValue() + f;
            return calcFloat(v);
        }
        case ParamValue::T_String:
        default:;
    }
    return current_;
}

ParamValue Parameter::calcFloat(float f) {
    switch (current_.type()) {
        case ParamValue::T_Float : {
            return ParamValue(f);
        }
        case ParamValue::T_String:
        default:;
    }
    return current_;
}

ParamValue Parameter::calcMidi(int midi) {
    float f = (float) midi / 127.0;
    return calcFloat(f);
}


bool Parameter::change(const ParamValue &c) {
    if (current_ != c) {
        current_ = c;
        return true;
    }
    return false;
}

void Parameter::dump() const {
    std::string d = id() + " : ";
    ParamValue cv = current();
    switch (cv.type()) {
        case ParamValue::T_Float :
            d += "  " + std::to_string(cv.floatValue()) + " [F],";
            break;
        case ParamValue::T_String :
        default:
            d += cv.stringValue() + " [S],";
            break;
    }
    LOG_1(d);
}

// Parameter_Float : [parameter] min max default
Parameter_Float::Parameter_Float(ParameterType type) : Parameter(type) {
    ;
}

void Parameter_Float::init(const std::vector<ParamValue> &args, unsigned &pos) {
    Parameter::init(args, pos);
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) min_ = args[pos++].floatValue();
    else
        throwError(id(), "missing min");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) max_ = args[pos++].floatValue();
    else
        throwError(id(), "missing max");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) def_ = args[pos++].floatValue();
    else
        throwError(id(), "missing def");
    change(def_);
}

void Parameter_Float::createArgs(std::vector<ParamValue> &args) const {
    Parameter::createArgs(args);
    args.push_back(ParamValue(min_));
    args.push_back(ParamValue(max_));
    args.push_back(ParamValue(def_));
}

std::string Parameter_Float::displayValue() const {
    char numbuf[11];
    sprintf(numbuf, "%.1f", current_.floatValue());
    return std::string(numbuf);
}


// Parameter_Float can assume float value
ParamValue Parameter_Float::calcRelative(float f) {
    float v = current().floatValue() + (f * (max() - min()));
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue(v);
}

ParamValue Parameter_Float::calcMidi(int midi) {
    float f = (float) midi / 127.0;
    return calcFloat(f);
}

ParamValue Parameter_Float::calcFloat(float f) {
    float v = (f * (max() - min())) + min();
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue(v);
}

bool Parameter_Float::change(const ParamValue &c) {
    switch (current_.type()) {
        case ParamValue::T_Float  : {
            float v = c.floatValue();
            v = std::max(v, min());
            v = std::min(v, max());
            return Parameter::change(ParamValue(v));
        }
        case ParamValue::T_String:
        default:;
    }
    return false;
}

// Parameter_Boolean :  [parameter] default
Parameter_Boolean::Parameter_Boolean(ParameterType type) : Parameter(type) {
    ;
}


std::string Parameter_Boolean::displayValue() const {
    if (current_.floatValue() > 0.5) {
        return "on";
    } else {
        return "off";
    }
}

void Parameter_Boolean::init(const std::vector<ParamValue> &args, unsigned &pos) {
    Parameter::init(args, pos);
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) {
        def_ = args[pos++].floatValue() > 0.5;
        change(def_);
    } else
        throwError(id(), "missing def");
}

void Parameter_Boolean::createArgs(std::vector<ParamValue> &args) const {
    Parameter::createArgs(args);
    args.push_back((float) def_);
}

bool Parameter_Boolean::change(const ParamValue &c) {
    switch (current_.type()) {
        case ParamValue::T_Float  : {
            float v = c.floatValue() > 0.5 ? 1.0 : 0.0;
            return Parameter::change(ParamValue(v));
        }
        case ParamValue::T_String:
        default:;
    }
    return false;
}

ParamValue Parameter_Boolean::calcRelative(float f) {
    if (current_.floatValue() > 0.5 && f < -0.0001) {
        return ParamValue(0.0);
    }
    if (current_.floatValue() <= 0.5 && f > 0.0001) {
        return ParamValue(1.0);
    }
    return current_;
}

ParamValue Parameter_Boolean::calcFloat(float f) {
    return ParamValue(f > 0.5 ? 1.0 : 0.0);
}

ParamValue Parameter_Boolean::calcMidi(int midi) {
    return ParamValue(midi > 63 ? 1.0 : 0.0);;
}


// simple derivative types
const std::string &Parameter_Percent::displayUnit() const {
    static std::string sUnit = "%";
    return sUnit;
}

const std::string &Parameter_Frequency::displayUnit() const {
    static std::string sUnit = "Hz";
    return sUnit;
}

const std::string &Parameter_Time::displayUnit() const {
    static std::string sUnit = "mSec";
    return sUnit;
}


//TODO : add a new T_Int type, wil be useful for ordinals too
void Parameter_Int::init(const std::vector<ParamValue> &args, unsigned &pos) {
    Parameter::init(args, pos);
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) min_ = args[pos++].floatValue();
    else
        throwError(id(), "missing min");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) max_ = args[pos++].floatValue();
    else
        throwError(id(), "missing max");
    if (args.size() > pos && args[pos].type() == ParamValue::T_Float) def_ = args[pos++].floatValue();
    else
        throwError(id(), "missing def");
    change(def_);
}

void Parameter_Int::createArgs(std::vector<ParamValue> &args) const {
    Parameter::createArgs(args);
    args.push_back(ParamValue(min_));
    args.push_back(ParamValue(max_));
    args.push_back(ParamValue(def_));
}

std::string Parameter_Int::displayValue() const {
    char numbuf[11];
    sprintf(numbuf, "%d", (int) current_.floatValue());
    return std::string(numbuf);
}


bool Parameter_Int::change(const ParamValue &c) {
    switch (current_.type()) {
        case ParamValue::T_Float  : {
            int v = c.floatValue();
            v = std::max(v, min());
            v = std::min(v, max());
            return Parameter::change(ParamValue((float) v));
        }
        case ParamValue::T_String:
        default:;
    }
    return false;
}

ParamValue Parameter_Int::calcRelative(float f) {
    int v = current().floatValue() + (f * (max() - min()));
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue((float) v);
}

ParamValue Parameter_Int::calcMidi(int midi) {
    float f = (float) midi / 127.0;
    int v = (f * (max() - min())) + min();
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue((float) v);
}

ParamValue Parameter_Int::calcFloat(float f) {
    int v = (f * (max() - min())) + min();
    v = std::max(v, min());
    v = std::min(v, max());
    return ParamValue((float) v);
}

const std::string &Parameter_Pitch::displayUnit() const {
    static std::string sUnit = "st";
    return sUnit;
}


} //namespace
