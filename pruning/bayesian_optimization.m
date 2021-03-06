function results = bayesian_optimization(n_iter, init_points, input_caffemodel, last_constraint, ...
                                         current_constraint, output_prefix, original_latency, constraint_type, ...
                                         cbo, tradeoff_factor, network, dataset, look_ahead)
    [~, ~, isloaded] = pyversion;
    if ~isloaded
        pyversion /local-scratch/changan-home/.pyenv/versions/py2/bin/python
    end
    py_path = '/local-scratch/changan-home/SkimCaffe/pruning/local-scratch/changan-home/SkimCaffe';
    sys_path = py.sys.path;
    if count(sys_path, py_path) == 0
        sys_path.append(py_path);
    end
    mod = py.importlib.import_module('pruning');
    py.reload(mod);

    if strcmp(network, 'alexnet')
        conv1 = optimizableVariable('conv1', [0, 1]);
        conv2 = optimizableVariable('conv2', [0, 1]);
        conv3 = optimizableVariable('conv3', [0, 1]);
        conv4 = optimizableVariable('conv4', [0, 1]);
        conv5 = optimizableVariable('conv5', [0, 1]);
        fc6 = optimizableVariable('fc6', [0, 1]);
        fc7 = optimizableVariable('fc7', [0, 1]);
        fc8 = optimizableVariable('fc8', [0, 1]);
        if strcmp(constraint_type, 'latency')
            parameters = [conv2, conv3, conv4, conv5, fc6, fc7, fc8];
        else
            parameters = [conv1, conv2, conv3, conv4, conv5, fc6, fc7, fc8];
        end
    elseif strcmp(network, 'resnet')
        conv2 = optimizableVariable('conv2', [0, 1]);
        conv3 = optimizableVariable('conv3', [0, 1]);
        conv4 = optimizableVariable('conv4', [0, 1]);
        conv5 = optimizableVariable('conv5', [0, 1]);
        parameters = [conv2, conv3, conv4, conv5];
    elseif strcmp(network, 'googlenet')
        conv2 = optimizableVariable('conv2', [0, 1]);
        i3a = optimizableVariable('i3a', [0, 1]);
        i3b = optimizableVariable('i3b', [0, 1]);
        i4a = optimizableVariable('i4a', [0, 1]);
        i4b = optimizableVariable('i4b', [0, 1]);
        i4c = optimizableVariable('i4c', [0, 1]);
        i4d = optimizableVariable('i4d', [0, 1]);
        i4e = optimizableVariable('i4e', [0, 1]);
        i5a = optimizableVariable('i5a', [0, 1]);
        i5b = optimizableVariable('i5b', [0, 1]);
        parameters = [conv2, i3a, i3b, i4a, i4b, i4c, i4d, i4e, i5a, i5b];
    else
        assert(true)
    end

    if cbo
        fun = @(input_params)constrained_bo(input_params, input_caffemodel, last_constraint, ...
                                            current_constraint, output_prefix, original_latency, ...
                                            constraint_type, cbo, tradeoff_factor, network, dataset, look_ahead);
        if strcmp(constraint_type, 'latency')
            % due to non-stable cpu environment
            results = bayesopt(fun, parameters, 'NumCoupledConstraints', 1, 'ExplorationRatio', 0.5, ...
                'AcquisitionFunctionName', 'expected-improvement-plus', 'Verbose', 1, ...
                'MaxObjectiveEvaluations', n_iter, 'NumSeedPoints', init_points, ...
                'AreCoupledConstraintsDeterministic', [false]);
        else
            results = bayesopt(fun, parameters, 'NumCoupledConstraints', 1, 'ExplorationRatio', 0.5, ...
                'AcquisitionFunctionName', 'expected-improvement-plus', 'Verbose', 1, ...
                'MaxObjectiveEvaluations', n_iter, 'NumSeedPoints', init_points);
        end
    else
        fun = @(input_params)unconstrained_bo(input_params, input_caffemodel, last_constraint, ...
                                              current_constraint, output_prefix, original_latency, ...
                                              constraint_type, cbo, tradeoff_factor, network, dataset, look_ahead);
        results = bayesopt(fun, parameters, 'ExplorationRatio', 0.5, ...
            'AcquisitionFunctionName', 'expected-improvement-plus', 'Verbose', 1, ...
            'MaxObjectiveEvaluations', n_iter, 'NumSeedPoints', init_points);
    end
    results = 1
end


function [objective, constraint] = constrained_bo(P, input_caffemodel, last_constraint, ...
                                                  current_constraint, output_prefix, original_latency, ...
                                                  constraint_type, cbo, tradeoff_factor, network, dataset, look_ahead)
    objective_func = py.pruning.objective_functions.matlab_objective_function(...
        input_caffemodel, last_constraint, current_constraint, output_prefix, original_latency, constraint_type, ...
        cbo, tradeoff_factor, network, dataset, look_ahead);

    if strcmp(network, 'alexnet')
        if strcmp(constraint_type, 'latency')
            kwa = pyargs('conv1', 0, 'conv2', P.conv2, 'conv3', P.conv3, 'conv4', P.conv4, ...
                'conv5', P.conv5, 'fc6', P.fc6, 'fc7', P.fc7, 'fc8', P.fc8);
        else
            kwa = pyargs('conv1', P.conv1, 'conv2', P.conv2, 'conv3', P.conv3, 'conv4', P.conv4, ...
                'conv5', P.conv5, 'fc6', P.fc6, 'fc7', P.fc7, 'fc8', P.fc8);
        end
    elseif strcmp(network, 'resnet')
        kwa = pyargs('conv2', P.conv2, 'conv3', P.conv3, 'conv4', P.conv4, 'conv5', P.conv5);
    elseif strcmp(network, 'googlenet')
        kwa = pyargs('conv2', P.conv2, 'i3a', P.i3a, 'i3b', P.i3b, 'i4a', P.i4a, 'i4b', P.i4b, 'i4c', P.i4c, ...
                     'i4d', P.i4d, 'i4e', P.i4e, 'i5a', P.i5a, 'i5b', P.i5b);
    else
        assert(true)
    end

    results = objective_func(kwa);
    objective = results{1};
    constraint = results{2};
end

function objective = unconstrained_bo(P, input_caffemodel, last_constraint, ...
                                      current_constraint, output_prefix, original_latency, ...
                                      constraint_type, cbo, tradeoff_factor, network, dataset, look_ahead)
    objective_func = py.pruning.objective_functions.matlab_objective_function(...
        input_caffemodel, last_constraint, current_constraint, output_prefix, original_latency, constraint_type, ...
        cbo, tradeoff_factor, network, dataset, look_ahead);

    if strcmp(network, 'alexnet')
        if strcmp(constraint_type, 'latency')
            kwa = pyargs('conv1', 0, 'conv2', P.conv2, 'conv3', P.conv3, 'conv4', P.conv4, ...
                'conv5', P.conv5, 'fc6', P.fc6, 'fc7', P.fc7, 'fc8', P.fc8);
        else
            kwa = pyargs('conv1', P.conv1, 'conv2', P.conv2, 'conv3', P.conv3, 'conv4', P.conv4, ...
                'conv5', P.conv5, 'fc6', P.fc6, 'fc7', P.fc7, 'fc8', P.fc8);
        end
    elseif strcmp(network, 'resnet')
        kwa = pyargs('conv2', P.conv2, 'conv3', P.conv3, 'conv4', P.conv4, 'conv5', P.conv5);
    elseif strcmp(network, 'googlenet')
        kwa = pyargs('conv2', P.conv2, 'i3a', P.i3a, 'i3b', P.i3b, 'i4a', P.i4a, 'i4b', P.i4b, 'i4c', P.i4c, ...
                     'i4d', P.i4d, 'i4e', P.i4e, 'i5a', P.i5a, 'i5b', P.i5b);
    else
        assert(true)
    end

    objective = objective_func(kwa);
end
