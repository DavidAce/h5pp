# API Reference


## class h5pp::File

```{eval-rst}
.. doxygenclass:: h5pp::v2::File
   :project: h5pp
   :undoc-members:
   :members:
```

## h5pp::Dataset

`Dataset` and `ConstDataset` are aliases of `BasicDataset` for mutable and const files.

```{eval-rst}
.. doxygenclass:: h5pp::v2::BasicDataset
   :project: h5pp
   :undoc-members:
   :members:
```

## h5pp::Attribute

`Attribute` and `ConstAttribute` are aliases of `BasicAttribute` for mutable and const files.

```{eval-rst}
.. doxygenclass:: h5pp::v2::BasicAttribute
   :project: h5pp
   :undoc-members:
   :members:
```

## h5pp::Table

`Table` and `ConstTable` are aliases of `BasicTable` for mutable and const files.

```{eval-rst}
.. doxygenclass:: h5pp::v2::BasicTable
   :project: h5pp
   :undoc-members:
   :members:
```

## h5pp::Group

`Group` and `ConstGroup` are aliases of `BasicGroup` for mutable and const files.

```{eval-rst}
.. doxygenclass:: h5pp::v2::BasicGroup
   :project: h5pp
   :undoc-members:
   :members:
```

## h5pp::Link

`Link` and `ConstLink` are aliases of `BasicLink` for mutable and const files.

```{eval-rst}
.. doxygenclass:: h5pp::v2::BasicLink
   :project: h5pp
   :undoc-members:
   :members:
```

## Operation options

### h5pp::DatasetCreateOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::DatasetCreateOptions
   :project: h5pp
   :members:
   :undoc-members:
```

### h5pp::DatasetWriteOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::DatasetWriteOptions
   :project: h5pp
   :members:
   :undoc-members:
```

### h5pp::DatasetAppendOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::DatasetAppendOptions
   :project: h5pp
   :members:
   :undoc-members:
```

### h5pp::DatasetReadOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::DatasetReadOptions
   :project: h5pp
   :members:
   :undoc-members:
```

### h5pp::AttributeWriteOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::AttributeWriteOptions
   :project: h5pp
   :members:
   :undoc-members:
```

### h5pp::AttributeReadOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::AttributeReadOptions
   :project: h5pp
   :members:
   :undoc-members:
```

### h5pp::TableCreateOptions

```{eval-rst}
.. doxygenstruct:: h5pp::v2::TableCreateOptions
   :project: h5pp
   :members:
   :undoc-members:
```

## enum h5pp::FileAccess

```{eval-rst}
.. doxygenenum:: h5pp::FileAccess
   :project: h5pp
```

## enum h5pp::TableSelection

```{eval-rst}
.. doxygenenum:: h5pp::TableSelection
   :project: h5pp
```


## enum h5pp::ResizePolicy

```{eval-rst}
.. doxygenenum:: h5pp::ResizePolicy
   :project: h5pp
```

## enum h5pp::LocationMode

```{eval-rst}
.. doxygenenum:: h5pp::LocationMode
   :project: h5pp
```

## class h5pp::v1::File

See [Migration](index.md#moving-from-h5pp-v1-to-v2) for using this class with existing code.

```{eval-rst}
.. doxygenclass:: h5pp::v1::File
   :project: h5pp
   :undoc-members:
   :members:
```
