#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/descriptor.h"


class UclProtoGenerator: public google::protobuf::compiler::CodeGenerator {
public:
    bool Generate(const google::protobuf::FileDescriptor* file,
            const std::string& parameter,
            google::protobuf::compiler::GeneratorContext* generator_context,
            std::string* error) const override {
        (void) parameter;
        static const std::string ProtoSuffix(".proto");
        std::size_t suffix_pos = file->name().rfind(ProtoSuffix);
        if (suffix_pos == std::string::npos) {
            error->append("unsupport proto src file\n");
            return false;
        }
        std::string file_pb_h = file->name().substr(0, suffix_pos).append(".pb.h");
        std::string file_pb_cc = file->name().substr(0, suffix_pos).append(".pb.cc");

        // include ucl header
        header_include_inject(file_pb_h, generator_context);
        
        int msg_count = file->message_type_count();
        for (int i = 0; i < msg_count; ++i) {
            const auto* descriptor = file->message_type(i);
            header_message_inject(file_pb_h, descriptor, generator_context);
            implement_message_inject(file_pb_cc, descriptor, generator_context);
        }

        return true;
    }

private:
    void header_include_inject(const std::string& file_name,
            google::protobuf::compiler::GeneratorContext* ctx) const {
        google::protobuf::io::Printer printer(
                ctx->OpenForInsert(file_name, "includes"), '$');
        std::map<std::string, std::string> vars;
        vars["libucl_dir"] = UCL_INCLUDE_DIR;
        printer.Print(vars, "#include \"$libucl_dir$/ucl++.h\"\n");
    }

    void header_message_inject(const std::string& file_name,
            const google::protobuf::Descriptor* descriptor,
            google::protobuf::compiler::GeneratorContext* ctx) const {
        std::string insertion_point("class_scope:");
        insertion_point.append(descriptor->full_name());

        google::protobuf::io::Printer printer(
                ctx->OpenForInsert(file_name, insertion_point), '$');
        std::map<std::string, std::string> vars;
        printer.Print(vars, "int ucl_load(const std::string& file_name, std::string& err) {\n");
        printer.Print(vars, "\treturn ucl_load(ucl::Ucl::parse_from_file(file_name, err), err);\n");
        printer.Print(vars, "}\n");
        printer.Print(vars, "int ucl_load(const ucl::Ucl& ucl_obj, std::string& err);\n");
    }

    void implement_message_inject(const std::string& file_name,
            const google::protobuf::Descriptor* descriptor,
            google::protobuf::compiler::GeneratorContext* ctx) const {
        std::string insertion_point("namespace_scope");

        google::protobuf::io::Printer printer(
                ctx->OpenForInsert(file_name, insertion_point), '$');
        std::map<std::string, std::string> vars;
        vars["class_name"] = descriptor->name();
        printer.Print(vars, "int $class_name$::ucl_load(const ucl::Ucl& ucl_obj, std::string& err) {\n");
        int field_count = descriptor->field_count();
        if (field_count == 0) {
            printer.Print(vars, "\t(void) err;\n");
        }
        printer.Print(vars, "\tif (ucl_obj.type() == UCL_NULL) { return -1; }\n\n");
        for (int i = 0; i < field_count; ++i) {
            const auto* field_desc = descriptor->field(i);
            vars["field_name"] = field_desc->name();
            std::string field_type_name;
            if (field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                field_type_name = field_desc->message_type()->full_name();
            } else if (field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
                field_type_name = field_desc->enum_type()->full_name();
            }
            while (true) {
                std::size_t dot_pos = field_type_name.find('.');
                if (dot_pos == std::string::npos) { break; }
                field_type_name.replace(dot_pos, 1, "::");
            }
            vars["field_type_name"] = field_type_name;
            vars["cpp_type_name"] = field_desc->cpp_type_name();

            printer.Print(vars, "\t// >>> set field: $field_name$ <<<\n");
            printer.Print(vars, "\tclear_$field_name$();\n");
            printer.Print(vars, "\tconst ucl::Ucl& u_$field_name$ = ucl_obj[\"$field_name$\"];\n");
            switch (field_desc->cpp_type()) {
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    if (field_desc->is_repeated()) {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_ARRAY) {\n");
                        printer.Print(vars, "\t\tsize_t rcnt = u_$field_name$.size();\n");
                        printer.Print(vars, "\t\tfor (size_t ri = 0; ri < rcnt; ++ri) {\n");
                        printer.Print(vars, "\t\t\tif (u_$field_name$[ri].type() != UCL_INT) {\n");
                        printer.Print(vars, "\t\t\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] item[\").append(std::to_string(ri)).append(\"] is a illegal value!\\n\");\n");
                        printer.Print(vars, "\t\t\t\treturn -1;\n");
                        printer.Print(vars, "\t\t\t}\n");
                        printer.Print(vars, "\t\t\tadd_$field_name$(u_$field_name$[ri].int_value());\n");
                        printer.Print(vars, "\t\t}\n");
                        printer.Print(vars, "\t} else {\n");
                        printer.Print(vars, "\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] should be a array!\\n\");\n");
                        printer.Print(vars, "\t\treturn -1;\n");
                        printer.Print(vars, "\t}\n");
                    } else {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_INT) {\n");
                        printer.Print(vars, "\t\tset_$field_name$(u_$field_name$.int_value());\n");
                        if (field_desc->is_required()) {
                            printer.Print(vars, "\t} else {\n");
                            printer.Print(vars, "\t\terr.append(\"[$field_name$: $cpp_type_name$] is required!\\n\");\n");
                            printer.Print(vars, "\t\treturn -1;\n");
                        }
                        printer.Print(vars, "\t}\n");
                    }
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    if (field_desc->is_repeated()) {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_ARRAY) {\n");
                        printer.Print(vars, "\t\tsize_t rcnt = u_$field_name$.size();\n");
                        printer.Print(vars, "\t\tfor (size_t ri = 0; ri < rcnt; ++ri) {\n");
                        printer.Print(vars, "\t\t\tif (u_$field_name$[ri].type() != UCL_FLOAT) {\n");
                        printer.Print(vars, "\t\t\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] item[\").append(std::to_string(ri)).append(\"] is a illegal value!\\n\");\n");
                        printer.Print(vars, "\t\t\t\treturn -1;\n");
                        printer.Print(vars, "\t\t\t}\n");
                        printer.Print(vars, "\t\t\tadd_$field_name$(u_$field_name$[ri].number_value());\n");
                        printer.Print(vars, "\t\t}\n");
                        printer.Print(vars, "\t} else {\n");
                        printer.Print(vars, "\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] should be a array!\\n\");\n");
                        printer.Print(vars, "\t\treturn -1;\n");
                        printer.Print(vars, "\t}\n");
                    } else {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_FLOAT) {\n");
                        printer.Print(vars, "\t\tset_$field_name$(u_$field_name$.number_value());\n");
                        if (field_desc->is_required()) {
                            printer.Print(vars, "\t} else {\n");
                            printer.Print(vars, "\t\terr.append(\"[$field_name$: $cpp_type_name$] is required!\\n\");\n");
                            printer.Print(vars, "\t\treturn -1;\n");
                        }
                        printer.Print(vars, "\t}\n");
                    }
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    if (field_desc->is_repeated()) {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_ARRAY) {\n");
                        printer.Print(vars, "\t\tsize_t rcnt = u_$field_name$.size();\n");
                        printer.Print(vars, "\t\tfor (size_t ri = 0; ri < rcnt; ++ri) {\n");
                        printer.Print(vars, "\t\t\tif (u_$field_name$[ri].type() == UCL_BOOLEAN) {\n");
                        printer.Print(vars, "\t\t\t\tadd_$field_name$(u_$field_name$[ri].bool_value());\n");
                        printer.Print(vars, "\t\t\t} else if (u_$field_name$[ri].type() == UCL_INT) {\n");
                        printer.Print(vars, "\t\t\t\tadd_$field_name$(bool(u_$field_name$[ri].int_value()));\n");
                        printer.Print(vars, "\t\t\t} else {\n");
                        printer.Print(vars, "\t\t\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] item[\").append(std::to_string(ri)).append(\"] is a illegal value!\\n\");\n");
                        printer.Print(vars, "\t\t\t\treturn -1;\n");
                        printer.Print(vars, "\t\t\t}\n");
                        printer.Print(vars, "\t\t}\n");
                        printer.Print(vars, "\t} else {\n");
                        printer.Print(vars, "\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] should be a array!\\n\");\n");
                        printer.Print(vars, "\t\treturn -1;\n");
                        printer.Print(vars, "\t}\n");
                    } else {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_BOOLEAN) {\n");
                        printer.Print(vars, "\t\tset_$field_name$(u_$field_name$.bool_value());\n");
                        printer.Print(vars, "\t} else if (u_$field_name$.type() == UCL_INT) {\n");
                        printer.Print(vars, "\t\tset_$field_name$(bool(u_$field_name$.int_value()));\n");
                        if (field_desc->is_required()) {
                            printer.Print(vars, "\t} else {\n");
                            printer.Print(vars, "\t\terr.append(\"[$field_name$: $cpp_type_name$] is required!\\n\");\n");
                            printer.Print(vars, "\t\treturn -1;\n");
                        }
                        printer.Print(vars, "\t}\n");
                    }
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    if (field_desc->is_repeated()) {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_ARRAY) {\n");
                        printer.Print(vars, "\t\tsize_t rcnt = u_$field_name$.size();\n");
                        printer.Print(vars, "\t\tfor (size_t ri = 0; ri < rcnt; ++ri) {\n");
                        printer.Print(vars, "\t\t\tif (u_$field_name$[ri].type() != UCL_INT || !$field_type_name$_IsValid(u_$field_name$[ri].int_value())) {\n");
                        printer.Print(vars, "\t\t\t\terr.append(\"[$field_name$: array<$field_type_name$>] item[\").append(std::to_string(ri)).append(\"] is a illegal value!\\n\");\n");
                        printer.Print(vars, "\t\t\t\treturn -1;\n");
                        printer.Print(vars, "\t\t\t}\n");
                        printer.Print(vars, "\t\t\tadd_$field_name$($field_type_name$(u_$field_name$[ri].int_value()));\n");
                        printer.Print(vars, "\t\t}\n");
                        printer.Print(vars, "\t} else {\n");
                        printer.Print(vars, "\t\terr.append(\"[$field_name$: array<$field_type_name$>] should be a array!\\n\");\n");
                        printer.Print(vars, "\t\treturn -1;\n");
                        printer.Print(vars, "\t}\n");
                    } else {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_INT && $field_type_name$_IsValid(u_$field_name$.int_value())) {\n");
                        printer.Print(vars, "\t\tset_$field_name$($field_type_name$(u_$field_name$.int_value()));\n");
                        if (field_desc->is_required()) {
                            printer.Print(vars, "\t} else {\n");
                            printer.Print(vars, "\t\terr.append(\"[$field_name$: $field_type_name$] is required!\\n\");\n");
                            printer.Print(vars, "\t\treturn -1;\n");
                        }
                        printer.Print(vars, "\t}\n");
                    }
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    if (field_desc->is_repeated()) {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_ARRAY) {\n");
                        printer.Print(vars, "\t\tsize_t rcnt = u_$field_name$.size();\n");
                        printer.Print(vars, "\t\tfor (size_t ri = 0; ri < rcnt; ++ri) {\n");
                        printer.Print(vars, "\t\t\tif (u_$field_name$[ri].type() != UCL_STRING) {\n");
                        printer.Print(vars, "\t\t\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] item[\").append(std::to_string(ri)).append(\"] is a illegal value!\\n\");\n");
                        printer.Print(vars, "\t\t\t\treturn -1;\n");
                        printer.Print(vars, "\t\t\t}\n");
                        printer.Print(vars, "\t\t\tadd_$field_name$(u_$field_name$[ri].string_value());\n");
                        printer.Print(vars, "\t\t}\n");
                        printer.Print(vars, "\t} else {\n");
                        printer.Print(vars, "\t\terr.append(\"[$field_name$: array<$cpp_type_name$>] should be a array!\\n\");\n");
                        printer.Print(vars, "\t\treturn -1;\n");
                        printer.Print(vars, "\t}\n");
                    } else {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_STRING) {\n");
                        printer.Print(vars, "\t\tset_$field_name$(u_$field_name$.string_value());\n");
                        if (field_desc->is_required()) {
                            printer.Print(vars, "\t} else {\n");
                            printer.Print(vars, "\t\terr.append(\"[$field_name$: $cpp_type_name$] is required!\\n\");\n");
                            printer.Print(vars, "\t\treturn -1;\n");
                        }
                        printer.Print(vars, "\t}\n");
                    }
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    if (field_desc->is_repeated()) {
                        printer.Print(vars, "\tif (u_$field_name$.type() == UCL_ARRAY) {\n");
                        printer.Print(vars, "\t\tsize_t rcnt = u_$field_name$.size();\n");
                        printer.Print(vars, "\t\tfor (size_t ri = 0; ri < rcnt; ++ri) {\n");
                        printer.Print(vars, "\t\t\t// $field_type_name$* tmp = add_$field_name$();\n");
                        printer.Print(vars, "\t\t\tif (add_$field_name$()->ucl_load(u_$field_name$[ri], err) != 0) {\n");
                        printer.Print(vars, "\t\t\t\terr.append(\"[$field_name$: array<$field_type_name$>] item[\").append(std::to_string(ri)).append(\"] is a illegal value!\\n\");\n");
                        printer.Print(vars, "\t\t\t\treturn -1;\n");
                        printer.Print(vars, "\t\t\t}\n");
                        printer.Print(vars, "\t\t}\n");
                        printer.Print(vars, "\t} else {\n");
                        printer.Print(vars, "\t\terr.append(\"[$field_name$: array<$field_type_name$>] should be a array!\\n\");\n");
                        printer.Print(vars, "\t\treturn -1;\n");
                        printer.Print(vars, "\t}\n");
                    } else {
                        printer.Print(vars, "\tif (mutable_$field_name$()->ucl_load(u_$field_name$, err) != 0) {\n");
                        if (field_desc->is_required()) {
                            printer.Print(vars, "\t\terr.append(\"[$field_name$: $field_type_name$] is required!\\n\");\n");
                            printer.Print(vars, "\t\treturn -1;\n");
                        } else {
                            printer.Print(vars, "\t\tclear_$field_name$();\n");
                        }
                        printer.Print(vars, "\t}\n");
                    }
                    break;
                default:
                    break;
            }

            printer.Print(vars, "\n");
        }
        printer.Print(vars, "\treturn 0;\n");
        printer.Print(vars, "}\n");
    } 
};

int main(int argc, char* argv[]) {
    UclProtoGenerator uclproto;
    return google::protobuf::compiler::PluginMain(argc, argv, &uclproto);
}

