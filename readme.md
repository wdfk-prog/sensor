# 传感器构建

[toc]

## 1. 文件结构

```c
└─Sensor
    ├─core
    │      rt_list.h
    │      sensor_builder.c
    │      sensor_builder.h
    │      sensor_default.c
    │      sensor_default.h
    │      sensor_driver.c
    │      sensor_driver.h
    │      sensor_group.c
    │      sensor_group.h
    │      sensor_register.c
    │
    └─driver
        ├─ads1015
        │      ads1015.c
        │      ads1015.h
        │
        ├─ds18b20
        │      ds18b20.c
        │      ds18b20.h
        │      sensor_18b20.c
        │      sensor_18b20.h
        │
        ├─mcs
        │      sensor_mcs.c
        │      sensor_mcs.h
        │
        ├─pt100
        │      sensor_pt100.c
        │      sensor_pt100.h
        │
        └─sht3x
                sensor_sht3x.c
                sensor_sht3x.h
                sht3x.c
                sht3x.h
```

## 2.使用方式

1. 编写 `sensor_register`函数,例如如下结构;并在应用代码中初始化该函数,用来注册传感器驱动

```c
/**
 * @brief  传感器注册函数
 * @note   None
 */
__weak void sensor_register(void)
{
   sensor_register_fun(&pt100[0].parent);
   sensor_register_fun(&pt100[1].parent);
   sensor_register_fun(&mcs.parent);
}
```

2. 编写传感器驱动,参考driver路径下已有传感器驱动编写
3. 在应用代码中将传感器添加至构建器中

```c
 builder_sensor_add(&ds18b20_builder, sensor);
```

4. 将应用配置添加至构建器中(可选)

```c
builder_config_add(&ds18b20_builder, &ds18b20_cfg, sizeof(ds18b20_cfg) / sizeof(struct sensor_default_cfg), true);//使用默认配置
```

5. 将构建器添加至执行构建中

```c
sensor_builder_add(&ds18b20_builder);
```

6. 完整流程参考example中例程

```c
//注册DS18B20传感器
sensor = sensor_obj_get("ds18b20");
if(sensor != NULL) {
    builder_sensor_add(&ds18b20_builder, sensor);
    builder_config_add(&ds18b20_builder, &ds18b20_cfg, sizeof(ds18b20_cfg) / sizeof(struct sensor_default_cfg), true);//使用默认配置
    ds18b20_cfg.unit = 10;//0.1C
    sensor_builder_add(&ds18b20_builder);
}

//初始化
sensor_director_init();

//运行
sensor_director_process();
```

## 3.构建器编写与使用

1. 默认提供default和group两种构建策略

default仅允许一个传感器顺序执行完成动作程序后再执行另一个传感器动作;group运行多个传感器顺序执行同一动作;

2. 定义传感器动作构建

```c
//传感器动作构建
static sensor_builder_t ds18b20_builder = 
{
    .allow_mode = false,	//每个任务都需要判断 false: 只在第一次执行判断
    .process = default_process,
    .process_num = sizeof(default_process) / sizeof(sensor_process_ops_t),
    .ops = &default_builder_ops,
};
```

如果默认提供动作策略无法满足要求,可自行实现添加动作

4. 添加动作程序

动作程序顺序执行,未实现中途变更顺序接口

allow函数用于当前动作是否执行判断

```c
static bool allow_collect(sensor_device_t sensor, void *cfg);
static sensor_process_ops_t default_process[] = 
{
    {   .allow      = &allow_collect,
        .handler    = &default_collect},
    {   .handler    = &default_calibration},
    {   .handler    = &default_range_check},
    {   .handler    = &default_data_check},
    {   .handler    = &default_alarm},
};
```

5. 添加应用配置

使用默认构建器提供的动作程序,使用默认构建器提供的配置结构体进行配置

若无法满足,可自行编写定义;

```c
static struct sensor_default_cfg ds18b20_cfg = 
{
    .power = DS18B20_CONSUME,
    .unit = 10,//0.1C
    .cal_addr = (uint32_t)&((user_config_t *)FLASH_USR_CONFIG_ADDR)->sensor_cfg[SENSOR_CFG_TEMP],
    .check = 
    {
        .max = 85,
        .min = -55,
    },
    .ops = 
    {
        .alarm_handler = temperature_alarm,
    }
};
```

## 4. 实现原理与编写目的

### 1. 编写目的

1. 实现传感器驱动的统一操作;实现传感器对象的统一管理;应用层无需关心底层传感器驱动的构建细节,既可编写传感器应

   sensor_driver.c实现,借鉴RTT的传感器驱动
2. 实现传感器基础应用编写的统一,提供通用策略给应用层调用;对于多个传感器,不同的传感器应用来说,调用提供的策略既可完成对于传感器基础应用的编写

### 2. 实现原理

1. 传感器驱动框架

![](readme.assets/%E4%BC%A0%E6%84%9F%E5%99%A8%E9%A9%B1%E5%8A%A8%E6%A1%86%E6%9E%B6.svg)

- 驱动内部定义配置信息,用于驱动运行与上下文变量的保存;需要返回给上层数据,由control函数编写提供支持
- 传感器注册使用链表进行驱动节点的添加;使用链表遍历进行传感器驱动对象的获取

2. 传感器构建框架

![](readme.assets/%E4%BC%A0%E6%84%9F%E5%99%A8%E6%9E%84%E5%BB%BA%E6%A1%86%E6%9E%B6.svg)

- 使用链表进行不同构建器的运行的遍历
- 使用数组形式,对不同传感器动作进行注册与调用;

比起链表更加直观,更容易了解运行顺序
