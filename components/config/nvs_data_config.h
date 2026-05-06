// DATA(name, type, default_value)
// DATA_ARRAY(name, type, array_size, default_value)
// SECTION_BEGIN(name)
// SECTION_END(name)

// obecnie wspierane typy: int32_t, uint8_t, float, double, char, char[]
// możliwość rozszerzenia wspieranych typów w pliku flash.c (należy na samym dole dodać parser i zaktualizować funkcje update_field oraz print_field_value)

#define CONFIG_FIELDS \
    DATA(device_id, uint8_t, 1) \
    DATA_ARRAY(device_str, char, 16, "ESP32") \
    SECTION_BEGIN(valve_cfg) \
    DATA(zero_offset_2, float, -0.65) \
    DATA(multiplier_2, float, 12.5) \
    DATA(calibration_value_1, int32_t, 12345) \
    SECTION_END(valve_cfg) \
    DATA(operation_mode, char, 'D')
// jeżeli ktokolwiek usunie tą linie to kompilator zacznie drzeć ryja (chyba że dodasz pustą linię po ostatniej definicji :)